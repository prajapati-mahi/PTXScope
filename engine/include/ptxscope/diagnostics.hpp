#pragma once

#include "ptxscope/common.hpp"
#include <string>
#include <vector>

namespace ptxscope {

enum class DiagnosticSeverity {
    ERROR,
    WARNING,
    OPTIMIZATION_SUGGESTION,
    NOTE
};

inline std::string severity_to_string(DiagnosticSeverity s) {
    switch (s) {
        case DiagnosticSeverity::ERROR:                   return "ERROR";
        case DiagnosticSeverity::WARNING:                 return "WARNING";
        case DiagnosticSeverity::OPTIMIZATION_SUGGESTION: return "OPTIMIZATION_SUGGESTION";
        case DiagnosticSeverity::NOTE:                    return "NOTE";
    }
    return "UNKNOWN";
}

struct Diagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::WARNING;
    std::string code;
    std::string message;
    SourceLocation location;
    uint32_t length = 1;
    std::string explanation;
    std::string suggested_action;

    std::string to_string() const;
    std::string to_json() const;
};

class DiagnosticEngine {
public:
    void report(Diagnostic diag);
    void report_error(std::string code, std::string msg, SourceLocation loc,
                      std::string explanation = "", std::string action = "");
    void report_warning(std::string code, std::string msg, SourceLocation loc,
                        std::string explanation = "", std::string action = "");
    void report_suggestion(std::string code, std::string msg, SourceLocation loc,
                           std::string explanation = "", std::string action = "");

    const std::vector<Diagnostic>& get_diagnostics() const { return diagnostics_; }
    bool has_errors() const;
    size_t error_count() const;
    size_t warning_count() const;
    size_t suggestion_count() const;

    void clear() { diagnostics_.clear(); }

private:
    std::vector<Diagnostic> diagnostics_;
};

} // namespace ptxscope
