#pragma once

#include <atomic>

static constexpr int SHUTDOWN_CODE = -6969;

inline std::atomic_bool should_shutdown {false};

