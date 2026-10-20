#include "ptxscope/diagnostics.hpp"
#include <sstream>

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

std::string Diagnostic::to_string() const {
    std::ostringstream ss;
    ss << "[" << severity_to_string(severity) << "] "
       << (code.empty() ? "" : "(" + code + ") ")
       << "line " << location.line << ":" << location.column << " - " << message;
    if (!explanation.empty()) {
        ss << "\n  Explanation: " << explanation;
    }
    if (!suggested_action.empty()) {
        ss << "\n  Suggested Action: " << suggested_action;
    }
    return ss.str();
}

std::string Diagnostic::to_json() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"severity\": \"" << severity_to_string(severity) << "\",\n"
       << "  \"code\": \"" << escape_json(code) << "\",\n"
       << "  \"message\": \"" << escape_json(message) << "\",\n"
       << "  \"line\": " << location.line << ",\n"
       << "  \"column\": " << location.column << ",\n"
       << "  \"length\": " << length << ",\n"
       << "  \"explanation\": \"" << escape_json(explanation) << "\",\n"
       << "  \"suggested_action\": \"" << escape_json(suggested_action) << "\"\n"
       << "}";
    return ss.str();
}

void DiagnosticEngine::report(Diagnostic diag) {
    diagnostics_.push_back(std::move(diag));
}

void DiagnosticEngine::report_error(std::string code, std::string msg, SourceLocation loc,
                                    std::string explanation, std::string action) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::ERROR;
    d.code = std::move(code);
    d.message = std::move(msg);
    d.location = loc;
    d.explanation = std::move(explanation);
    d.suggested_action = std::move(action);
    report(std::move(d));
}

void DiagnosticEngine::report_warning(std::string code, std::string msg, SourceLocation loc,
                                      std::string explanation, std::string action) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::WARNING;
    d.code = std::move(code);
    d.message = std::move(msg);
    d.location = loc;
    d.explanation = std::move(explanation);
    d.suggested_action = std::move(action);
    report(std::move(d));
}

void DiagnosticEngine::report_suggestion(std::string code, std::string msg, SourceLocation loc,
                                         std::string explanation, std::string action) {
    Diagnostic d;
    d.severity = DiagnosticSeverity::OPTIMIZATION_SUGGESTION;
    d.code = std::move(code);
    d.message = std::move(msg);
    d.location = loc;
    d.explanation = std::move(explanation);
    d.suggested_action = std::move(action);
    report(std::move(d));
}

bool DiagnosticEngine::has_errors() const {
    for (const auto& d : diagnostics_) {
        if (d.severity == DiagnosticSeverity::ERROR) return true;
    }
    return false;
}

size_t DiagnosticEngine::error_count() const {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.severity == DiagnosticSeverity::ERROR) count++;
    }
    return count;
}

size_t DiagnosticEngine::warning_count() const {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.severity == DiagnosticSeverity::WARNING) count++;
    }
    return count;
}

size_t DiagnosticEngine::suggestion_count() const {
    size_t count = 0;
    for (const auto& d : diagnostics_) {
        if (d.severity == DiagnosticSeverity::OPTIMIZATION_SUGGESTION) count++;
    }
    return count;
}

} // namespace ptxscope
