/*
 * Copyright (c) 2026 Jolsho
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

template <size_t NumBits>
class Bitmap {
public:
    static constexpr size_t BIT_SIZE   = NumBits;
    static constexpr size_t BYTE_SIZE  = NumBits / 8;

    Bitmap(const uint8_t* cursor = nullptr) {
        if (cursor) {
            std::memcpy(data.data(), cursor, BYTE_SIZE);
        } else {
            data.fill(0);
        }
    }

    bool is_set(size_t bit) const {
        check_index(bit);
        size_t byte_index = bit / 8;
        size_t bit_index  = bit % 8;
        return (data[byte_index] & (uint8_t(1) << bit_index)) != 0;
    }

    void set(size_t bit) {
        check_index(bit);
        size_t byte_index = bit / 8;
        size_t bit_index  = bit % 8;
        data[byte_index] |= (uint8_t(1) << bit_index);
    }

    void clear(size_t bit) {
        check_index(bit);
        size_t byte_index = bit / 8;
        size_t bit_index  = bit % 8;
        data[byte_index] &= ~(uint8_t(1) << bit_index);
    }

    void toggle(size_t bit) {
        check_index(bit);
        size_t byte_index = bit / 8;
        size_t bit_index  = bit % 8;
        data[byte_index] ^= (uint8_t(1) << bit_index);
    }

    size_t count() const {
        size_t c = 0;
        for (size_t i = 0; i < BIT_SIZE; i++) {
            if (is_set(i)) c++;
        }
        return c;
    }

    // raw pointer interface
    uint8_t* data_ptr() {
        return data.data();
    }

    const uint8_t* data_ptr() const {
        return data.data();
    }

    // access as array
    const std::array<uint8_t, BYTE_SIZE>& array() const {
        return data;
    }

private:
    std::array<uint8_t, BYTE_SIZE> data;

    void check_index(size_t bit) const {
        if (bit >= BIT_SIZE)
            throw std::out_of_range("Bit index out of range");
    }
};
