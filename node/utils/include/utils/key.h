#pragma once
#include <array>
#include <cstddef>

static constexpr size_t KEY_SIZE = 32;
using Key = std::array<unsigned char, KEY_SIZE>;
const Key ZERO_KEY = {0};
