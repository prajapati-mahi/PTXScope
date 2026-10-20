#include "ptxscope/ptxas_integration.hpp"
#include <sstream>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace ptxscope {

namespace {

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

} // namespace

std::string PtxasResult::to_json() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"available\": " << (available ? "true" : "false") << ",\n"
       << "  \"executable_path\": \"" << escape_json(executable_path) << "\",\n"
       << "  \"compilation_success\": " << (compilation_success ? "true" : "false") << ",\n"
       << "  \"exit_code\": " << exit_code << ",\n"
       << "  \"stdout\": \"" << escape_json(stdout_output) << "\",\n"
       << "  \"stderr\": \"" << escape_json(stderr_output) << "\",\n"
       << "  \"duration_ms\": " << duration_ms << ",\n"
       << "  \"target_arch\": \"" << escape_json(target_arch) << "\",\n"
       << "  \"status_message\": \"" << escape_json(status_message) << "\"\n"
       << "}";
    return ss.str();
}

bool PtxasIntegration::is_available(const std::string& custom_path) {
    return !find_executable(custom_path).empty();
}

std::string PtxasIntegration::find_executable(const std::string& custom_path) {
    if (!custom_path.empty() && std::filesystem::exists(custom_path)) {
        return custom_path;
    }

    // Check system PATH
    const char* path_env = std::getenv("PATH");
    if (!path_env) return "";

    std::string path_str(path_env);
    std::stringstream ss(path_str);
    std::string dir;
#ifdef _WIN32
    const char delimiter = ';';
    const std::string exe_name = "ptxas.exe";
#else
    const char delimiter = ':';
    const std::string exe_name = "ptxas";
#endif

    while (std::getline(ss, dir, delimiter)) {
        std::filesystem::path full = std::filesystem::path(dir) / exe_name;
        if (std::filesystem::exists(full)) {
            return full.string();
        }
    }

    // Check standard Windows CUDA install paths
#ifdef _WIN32
    std::vector<std::string> standard_dirs = {
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.6\\bin\\ptxas.exe",
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.5\\bin\\ptxas.exe",
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.4\\bin\\ptxas.exe",
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v12.0\\bin\\ptxas.exe",
        "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v11.8\\bin\\ptxas.exe"
    };
    for (const auto& p : standard_dirs) {
        if (std::filesystem::exists(p)) return p;
    }
#endif

    return "";
}

PtxasResult PtxasIntegration::validate(const std::string& ptx_source,
                                      const std::string& arch,
                                      const std::string& custom_path) {
    PtxasResult res;
    res.target_arch = arch;
    std::string ptxas_bin = find_executable(custom_path);

    if (ptxas_bin.empty()) {
        res.available = false;
        res.status_message = "NVIDIA ptxas compiler not found on host. Hardware compilation validation skipped.";
        return res;
    }

    res.available = true;
    res.executable_path = ptxas_bin;

    // Write ptx_source to a temporary file
    auto temp_dir = std::filesystem::temp_directory_path();
    auto temp_ptx = temp_dir / ("ptxscope_temp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".ptx");
    auto temp_out = temp_dir / ("ptxscope_temp_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()) + ".cubin");

    {
        std::ofstream ofs(temp_ptx);
        if (!ofs.is_open()) {
            res.status_message = "Failed to create temporary PTX file for validation.";
            return res;
        }
        ofs << ptx_source;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Command: ptxas --gpu-name <arch> <temp_ptx> -o <temp_out>
    std::string cmd = "\"" + ptxas_bin + "\" --gpu-name " + arch + " \"" + temp_ptx.string() + "\" -o \"" + temp_out.string() + "\" 2>&1";

    FILE* pipe = nullptr;
#ifdef _WIN32
    pipe = _popen(cmd.c_str(), "r");
#else
    pipe = popen(cmd.c_str(), "r");
#endif

    std::string output;
    if (pipe) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }
#ifdef _WIN32
        res.exit_code = _pclose(pipe);
#else
        int status = pclose(pipe);
        res.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
    } else {
        res.exit_code = -1;
        res.stderr_output = "Failed to execute ptxas process.";
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    res.duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    if (res.exit_code == 0) {
        res.compilation_success = true;
        res.stdout_output = output;
        res.status_message = "Successfully compiled and verified by NVIDIA ptxas for target " + arch + ".";
    } else {
        res.compilation_success = false;
        res.stderr_output = output;
        res.status_message = "ptxas compilation failed with exit code " + std::to_string(res.exit_code) + ".";
    }

    // Cleanup temp files
    std::error_code ec;
    std::filesystem::remove(temp_ptx, ec);
    std::filesystem::remove(temp_out, ec);

    return res;
}

} // namespace ptxscope
