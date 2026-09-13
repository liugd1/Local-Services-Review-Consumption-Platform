#pragma once
// 统一路径解析：以可执行文件为锚向上定位项目根，
// 保证无论从项目根 / build / 其它目录启动，public 与 data 都指向同一位置。
#include <filesystem>
#include <string>

namespace paths {

inline std::filesystem::path& rootRef() {
    static std::filesystem::path p = std::filesystem::current_path();
    return p;
}

// 传入 argv[0]，向上寻找最近含 CMakeLists.txt 的目录作为项目根
inline void init(const char* argv0) {
    std::error_code ec;
    std::filesystem::path exe = argv0 && *argv0
                                    ? std::filesystem::absolute(argv0, ec)
                                    : std::filesystem::current_path();
    auto dir = exe.has_parent_path() ? exe.parent_path() : std::filesystem::current_path();
    for (;;) {
        if (std::filesystem::exists(dir / "CMakeLists.txt", ec)) break;
        auto parent = dir.parent_path();
        if (parent == dir) break;
        dir = parent;
    }
    rootRef() = dir;
}

inline std::filesystem::path root() { return rootRef(); }
inline std::filesystem::path publicDir() { return root() / "public"; }
inline std::filesystem::path dataDir() { return root() / "data"; }

}  // namespace paths
