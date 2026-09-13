#pragma once
// 时间工具：统一使用 "YYYY-MM-DD HH:MM:SS" 字符串（可字典序比较）
#include <ctime>
#include <string>

namespace timeutil {

inline std::string formatTime(std::time_t t) {
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32] = {0};
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

inline std::time_t now() { return std::time(nullptr); }

inline std::string nowStr() { return formatTime(now()); }

// 当前时间 + days 天
inline std::string afterDaysStr(int days) {
    return formatTime(now() + static_cast<std::time_t>(days) * 86400);
}

// 当前时间 + seconds 秒
inline std::string afterSecondsStr(long long seconds) {
    return formatTime(now() + static_cast<std::time_t>(seconds));
}

inline long long nowUnix() { return static_cast<long long>(now()); }

}  // namespace timeutil
