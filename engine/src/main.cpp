#include "ptxscope/lexer.hpp"
#include "ptxscope/parser.hpp"
#include "ptxscope/cfg.hpp"
#include "ptxscope/analysis.hpp"
#include "ptxscope/optimizer.hpp"
#include "ptxscope/ptxas_integration.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

namespace {

std::string read_file_or_exit(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open file '" << path << "'\n";
        std::exit(1);
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

std::vector<std::string> split_comma(const std::string& s) {
    std::vector<std::string> res;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) res.push_back(item);
    }
    return res;
}

std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

void print_usage() {
    std::cout << "PTXScope - GPU Assembly Analyzer, Optimizer & Debugging Workbench\n"
              << "Usage: ptxscope <command> [options] <file.ptx>\n\n"
              << "Commands:\n"
              << "  parse <file>                        Tokenize and parse PTX assembly\n"
              << "  analyze <file>                      Run static analysis and diagnostics\n"
              << "  optimize <file> [--passes p1,p2]    Run optimization passes and output transformed PTX\n"
              << "  cfg <file> [--format dot|json]      Generate Control Flow Graph\n"
              << "  validate <file> [--gpu-name sm_XX]  Invoke NVIDIA ptxas if installed\n"
              << "  json <file> [--passes p1,p2]        Full structured JSON output for workbench integration\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    if (command == "--help" || command == "-h") {
        print_usage();
        return 0;
    }

    std::string filename;
    std::string format = "dot";
    std::string gpu_name = "sm_70";
    std::vector<std::string> passes;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--format" && i + 1 < argc) {
            format = argv[++i];
        } else if (arg == "--gpu-name" && i + 1 < argc) {
            gpu_name = argv[++i];
        } else if (arg == "--passes" && i + 1 < argc) {
            passes = split_comma(argv[++i]);
        } else if (arg.rfind("--", 0) != 0) {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No input PTX file specified.\n";
        return 1;
    }

    std::string source = read_file_or_exit(filename);

    ptxscope::DiagnosticEngine diag;
    ptxscope::Lexer lexer(source);
    auto tokens = lexer.tokenize();

    ptxscope::Parser parser(std::move(tokens), diag);
    auto mod = parser.parse_module();

    if (command == "parse") {
        std::cout << "PTXScope Parser Result for '" << filename << "':\n";
        std::cout << "Functions parsed: " << mod->functions.size() << "\n";
        std::cout << "Total instructions: " << mod->get_total_instructions() << "\n\n";

        for (const auto& fn : mod->functions) {
            std::cout << (fn->is_entry ? "Kernel .entry " : "Function .func ")
                      << fn->name << " (" << fn->blocks.size() << " basic blocks):\n";
            for (const auto& bb : fn->blocks) {
                std::cout << "  Block [" << bb->label << "] (" << bb->instructions.size() << " insts)\n";
                for (const auto& inst : bb->instructions) {
                    std::cout << "    " << inst->to_ptx() << "\n";
                }
            }
        }

        if (diag.has_errors() || diag.warning_count() > 0) {
            std::cout << "\nDiagnostics:\n";
            for (const auto& d : diag.get_diagnostics()) {
                std::cout << d.to_string() << "\n";
            }
        }
        return diag.has_errors() ? 1 : 0;
    }

    if (command == "cfg") {
        if (mod->functions.empty()) {
            std::cerr << "No functions found to generate CFG.\n";
            return 1;
        }
        auto fn = mod->functions.front();
        ptxscope::ControlFlowGraph cfg(fn);
        cfg.build();

        if (format == "json") {
            std::cout << cfg.to_json() << "\n";
        } else {
            std::cout << cfg.to_dot() << "\n";
        }
        return 0;
    }

    if (command == "analyze") {
        std::cout << "PTXScope Static Analysis for '" << filename << "':\n\n";
        for (const auto& fn : mod->functions) {
            ptxscope::StaticAnalyzer analyzer(fn, diag);
            analyzer.run_analysis();

            std::cout << "Function: " << fn->name << "\n";
            std::cout << "Register Census:\n";
            for (const auto& [reg, u] : analyzer.get_reg_usages()) {
                std::cout << "  " << reg << ": defs=" << u.def_count << ", uses=" << u.use_count << "\n";
            }

            std::cout << "\nBasic Block Liveness:\n";
            for (const auto& [lbl, bl] : analyzer.get_liveness()) {
                std::cout << "  [" << lbl << "]:\n";
                std::cout << "    Live-In:  ";
                for (const auto& r : bl.live_in) std::cout << r << " ";
                std::cout << "\n    Live-Out: ";
                for (const auto& r : bl.live_out) std::cout << r << " ";
                std::cout << "\n";
            }
        }

        std::cout << "\nDiagnostics (" << diag.get_diagnostics().size() << " total):\n";
        for (const auto& d : diag.get_diagnostics()) {
            std::cout << d.to_string() << "\n\n";
        }
        return 0;
    }

    if (command == "optimize") {
        auto pm = ptxscope::PassManager::create_standard_pipeline(passes);
        size_t initial_inst_count = mod->get_total_instructions();
        auto results = pm->run(*mod);
        size_t optimized_inst_count = mod->get_total_instructions();

        std::cout << "// PTXScope Optimized Output for '" << filename << "'\n";
        std::cout << "// Initial instructions: " << initial_inst_count
                  << ", Optimized instructions: " << optimized_inst_count << "\n\n";
        std::cout << mod->to_ptx() << "\n";

        std::cout << "\n--- Optimization Summary ---\n";
        for (const auto& pr : results) {
            std::cout << "Pass [" << pr.pass_name << "]: "
                      << pr.num_transformations << " transformations applied.\n";
            for (const auto& exp : pr.explanations) {
                std::cout << "  - " << exp << "\n";
            }
            for (const auto& skip : pr.skipped_reasons) {
                std::cout << "  * [skipped] " << skip << "\n";
            }
        }
        return 0;
    }

    if (command == "validate") {
        auto val = ptxscope::PtxasIntegration::validate(source, gpu_name);
        std::cout << "NVIDIA ptxas Validation Result:\n";
        std::cout << "  Available: " << (val.available ? "YES" : "NO") << "\n";
        if (val.available) {
            std::cout << "  Compiler:  " << val.executable_path << "\n";
            std::cout << "  Target:    " << val.target_arch << "\n";
            std::cout << "  Status:    " << (val.compilation_success ? "PASSED" : "FAILED") << "\n";
            std::cout << "  Time:      " << val.duration_ms << " ms\n";
            if (!val.stdout_output.empty()) std::cout << "  Stdout:\n" << val.stdout_output << "\n";
            if (!val.stderr_output.empty()) std::cout << "  Stderr:\n" << val.stderr_output << "\n";
        } else {
            std::cout << "  Note:      " << val.status_message << "\n";
        }
        return val.compilation_success ? 0 : (val.available ? 1 : 0);
    }

    if (command == "json") {
        // Complete structured JSON for web workbench
        auto start_time = std::chrono::high_resolution_clock::now();

        std::string original_ptx = source;
        size_t initial_inst_count = mod->get_total_instructions();

        // 1. Analyze functions
        std::ostringstream fn_analysis_json;
        fn_analysis_json << "[";
        for (size_t i = 0; i < mod->functions.size(); ++i) {
            const auto& fn = mod->functions[i];
            ptxscope::StaticAnalyzer analyzer(fn, diag);
            analyzer.run_analysis();
            fn_analysis_json << analyzer.to_json();
            if (i + 1 < mod->functions.size()) fn_analysis_json << ",";
        }
        fn_analysis_json << "]";

        // 2. CFG for primary function
        std::string cfg_json = "{}";
        if (!mod->functions.empty()) {
            ptxscope::ControlFlowGraph cfg(mod->functions.front());
            cfg.build();
            cfg_json = cfg.to_json();
        }

        // 3. Run Optimization
        auto pm = ptxscope::PassManager::create_standard_pipeline(passes);
        auto opt_results = pm->run(*mod);
        size_t optimized_inst_count = mod->get_total_instructions();
        std::string optimized_ptx = mod->to_ptx();

        // 4. Validate with ptxas
        auto ptxas_val = ptxscope::PtxasIntegration::validate(original_ptx, gpu_name);

        auto end_time = std::chrono::high_resolution_clock::now();
        double total_duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Print final combined JSON
        std::cout << "{\n"
                  << "  \"success\": " << (!diag.has_errors() ? "true" : "false") << ",\n"
                  << "  \"metrics\": {\n"
                  << "    \"initial_instruction_count\": " << initial_inst_count << ",\n"
                  << "    \"optimized_instruction_count\": " << optimized_inst_count << ",\n"
                  << "    \"basic_block_count\": " << (mod->functions.empty() ? 0 : mod->functions.front()->blocks.size()) << ",\n"
                  << "    \"function_count\": " << mod->functions.size() << ",\n"
                  << "    \"execution_duration_ms\": " << total_duration_ms << "\n"
                  << "  },\n"
                  << "  \"original_ptx\": \"" << escape_json(original_ptx) << "\",\n"
                  << "  \"optimized_ptx\": \"" << escape_json(optimized_ptx) << "\",\n"
                  << "  \"cfg\": " << cfg_json << ",\n"
                  << "  \"analysis\": " << fn_analysis_json.str() << ",\n"
                  << "  \"diagnostics\": [\n";

        const auto& all_diag = diag.get_diagnostics();
        for (size_t i = 0; i < all_diag.size(); ++i) {
            std::cout << "    " << all_diag[i].to_json();
            if (i + 1 < all_diag.size()) std::cout << ",";
            std::cout << "\n";
        }

        std::cout << "  ],\n"
                  << "  \"pass_results\": [\n";
        for (size_t i = 0; i < opt_results.size(); ++i) {
            std::cout << "    " << opt_results[i].to_json();
            if (i + 1 < opt_results.size()) std::cout << ",";
            std::cout << "\n";
        }

        std::cout << "  ],\n"
                  << "  \"ptxas_validation\": " << ptxas_val.to_json() << "\n"
                  << "}\n";
        return 0;
    }

    std::cerr << "Unknown command: '" << command << "'. Run 'ptxscope --help' for usage.\n";
    return 1;
}
