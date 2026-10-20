#pragma once

#include <string>
#include <optional>
#include <vector>

namespace ptxscope {

struct PtxasResult {
    bool available = false;
    std::string executable_path;
    int exit_code = -1;
    bool compilation_success = false;
    std::string stdout_output;
    std::string stderr_output;
    double duration_ms = 0.0;
    std::string target_arch;
    std::string status_message;

    std::string to_json() const;
};

class PtxasIntegration {
public:
    static bool is_available(const std::string& custom_path = "");
    static std::string find_executable(const std::string& custom_path = "");

    static PtxasResult validate(const std::string& ptx_source,
                                const std::string& arch = "sm_70",
                                const std::string& custom_path = "");
};

} // namespace ptxscope
