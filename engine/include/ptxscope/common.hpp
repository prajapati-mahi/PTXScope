#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <optional>
#include <ostream>

namespace ptxscope {

struct SourceLocation {
    uint32_t line = 1;
    uint32_t column = 1;

    bool operator==(const SourceLocation& other) const {
        return line == other.line && column == other.column;
    }
};

struct SourceSpan {
    SourceLocation start;
    SourceLocation end;
};

enum class DataType {
    NONE,
    PRED,
    U8, U16, U32, U64,
    S8, S16, S32, S64,
    B8, B16, B32, B64,
    F16, F32, F64
};

inline std::string data_type_to_string(DataType dt) {
    switch (dt) {
        case DataType::PRED: return ".pred";
        case DataType::U8:   return ".u8";
        case DataType::U16:  return ".u16";
        case DataType::U32:  return ".u32";
        case DataType::U64:  return ".u64";
        case DataType::S8:   return ".s8";
        case DataType::S16:  return ".s16";
        case DataType::S32:  return ".s32";
        case DataType::S64:  return ".s64";
        case DataType::B8:   return ".b8";
        case DataType::B16:  return ".b16";
        case DataType::B32:  return ".b32";
        case DataType::B64:  return ".b64";
        case DataType::F16:  return ".f16";
        case DataType::F32:  return ".f32";
        case DataType::F64:  return ".f64";
        default:             return "";
    }
}

inline DataType parse_data_type(std::string_view sv) {
    if (sv == ".pred") return DataType::PRED;
    if (sv == ".u8")   return DataType::U8;
    if (sv == ".u16")  return DataType::U16;
    if (sv == ".u32")  return DataType::U32;
    if (sv == ".u64")  return DataType::U64;
    if (sv == ".s8")   return DataType::S8;
    if (sv == ".s16")  return DataType::S16;
    if (sv == ".s32")  return DataType::S32;
    if (sv == ".s64")  return DataType::S64;
    if (sv == ".b8")   return DataType::B8;
    if (sv == ".b16")  return DataType::B16;
    if (sv == ".b32")  return DataType::B32;
    if (sv == ".b64")  return DataType::B64;
    if (sv == ".f16")  return DataType::F16;
    if (sv == ".f32")  return DataType::F32;
    if (sv == ".f64")  return DataType::F64;
    return DataType::NONE;
}

inline bool is_integer_type(DataType dt) {
    switch (dt) {
        case DataType::U8: case DataType::U16: case DataType::U32: case DataType::U64:
        case DataType::S8: case DataType::S16: case DataType::S32: case DataType::S64:
        case DataType::B8: case DataType::B16: case DataType::B32: case DataType::B64:
            return true;
        default:
            return false;
    }
}

inline bool is_floating_point_type(DataType dt) {
    return dt == DataType::F16 || dt == DataType::F32 || dt == DataType::F64;
}

inline uint32_t get_data_type_bitwidth(DataType dt) {
    switch (dt) {
        case DataType::PRED: return 1;
        case DataType::U8:  case DataType::S8:  case DataType::B8:  return 8;
        case DataType::U16: case DataType::S16: case DataType::B16: case DataType::F16: return 16;
        case DataType::U32: case DataType::S32: case DataType::B32: case DataType::F32: return 32;
        case DataType::U64: case DataType::S64: case DataType::B64: case DataType::F64: return 64;
        default: return 0;
    }
}

struct Predicate {
    bool is_predicated = false;
    bool is_negated = false;
    std::string reg_name; // e.g. "%p0"

    std::string to_string() const {
        if (!is_predicated) return "";
        return (is_negated ? "@!" : "@") + reg_name;
    }
};

enum class Opcode {
    UNKNOWN,
    // Directives
    DIR_VERSION,
    DIR_TARGET,
    DIR_ADDRESS_SIZE,
    DIR_ENTRY,
    DIR_FUNC,
    DIR_PARAM,
    DIR_REG,
    DIR_GLOBAL,
    DIR_SHARED,
    DIR_CONST,
    // Core Instructions
    MOV,
    ADD,
    SUB,
    MUL,
    MAD,
    SETP,
    LD,
    ST,
    BRA,
    RET,
    EXIT,
    BAR_SYNC
};

inline std::string opcode_to_string(Opcode op) {
    switch (op) {
        case Opcode::MOV:      return "mov";
        case Opcode::ADD:      return "add";
        case Opcode::SUB:      return "sub";
        case Opcode::MUL:      return "mul";
        case Opcode::MAD:      return "mad";
        case Opcode::SETP:     return "setp";
        case Opcode::LD:       return "ld";
        case Opcode::ST:       return "st";
        case Opcode::BRA:      return "bra";
        case Opcode::RET:      return "ret";
        case Opcode::EXIT:     return "exit";
        case Opcode::BAR_SYNC: return "bar.sync";
        default:               return "unknown";
    }
}

inline std::ostream& operator<<(std::ostream& os, DataType dt) {
    return os << data_type_to_string(dt);
}

inline std::ostream& operator<<(std::ostream& os, Opcode op) {
    return os << opcode_to_string(op);
}

} // namespace ptxscope
