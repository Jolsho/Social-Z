#pragma  once
#include <cstdint>

enum class LogCode : uint8_t {
    Log,
};

constexpr uint8_t log_code(LogCode c) { return static_cast<uint8_t>(c); }
