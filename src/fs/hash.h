#include <array>
#include <cstddef>
#include <functional>


struct HashArray16 {
    size_t operator()(const std::array<std::byte, 16>& arr) const noexcept {
        std::size_t h = 0;
        for (auto b : arr) {
            h ^= std::hash<std::byte>{}(b) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};
