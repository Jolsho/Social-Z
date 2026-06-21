#pragma once
#include <filesystem>

static constexpr std::string APP = "TUIE";

struct AppPaths {
    std::filesystem::path config_dir;
    std::filesystem::path data_dir;
    std::filesystem::path log_dir;
    std::filesystem::path run_dir;

};

inline std::string cpy_apnd(const std::filesystem::path& root_const, std::string&& ext) {
    std::string root = root_const;
    root.append(ext);
    return root;
}

inline AppPaths resolve_paths() {
#ifdef __linux__
    return {
        "/etc/" + APP,
        "/var/lib/" + APP,
        "/var/log/" + APP,
        "/run/" + APP
    };
#elif _WIN32
    const char* base = std::getenv("ProgramData");
    std::filesystem::path root = base ? base : "C:/ProgramData";
    return {
        root / APP / "config",
        root / APP,
        root / APP / "logs",
        root / APP / "run"
    };
#endif
}

static const AppPaths PATHS = resolve_paths();
