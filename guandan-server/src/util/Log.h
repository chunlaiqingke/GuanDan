#pragma once

#include <string>

// 有 spdlog 时用 spdlog（项目规范要求），否则退回极简 {} 占位符格式化，保证可编译。
#if defined(__has_include)
#  if __has_include(<spdlog/spdlog.h>)
#    define GUANDAN_HAS_SPDLOG 1
#  endif
#endif

#if GUANDAN_HAS_SPDLOG
#  include <spdlog/spdlog.h>
namespace guandan::util {
  inline void initLog() {
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
  }
}  // namespace guandan::util
#  define GD_LOG_TRACE(...) ::spdlog::trace(__VA_ARGS__)
#  define GD_LOG_DEBUG(...) ::spdlog::debug(__VA_ARGS__)
#  define GD_LOG_INFO(...)  ::spdlog::info(__VA_ARGS__)
#  define GD_LOG_WARN(...)  ::spdlog::warn(__VA_ARGS__)
#  define GD_LOG_ERROR(...) ::spdlog::error(__VA_ARGS__)
#else
#  include <cstdio>
#  include <sstream>
namespace guandan::util {
  inline void initLog() {}
  // 极简 {} 占位符格式化（仅无 spdlog 时使用）。
  template <typename... Args>
  inline std::string format(std::string fmt, Args&&... args) {
    std::string out;
    size_t pos = 0;
    auto append = [&](const auto& v) {
      const size_t p = fmt.find("{}", pos);
      if (p == std::string::npos) return;
      out += fmt.substr(pos, p - pos);
      std::ostringstream oss;
      oss << v;
      out += oss.str();
      pos = p + 2;
    };
    (append(args), ...);
    out += fmt.substr(pos);
    return out;
  }
}  // namespace guandan::util
#  define GD_LOG_IMPL(level, fmt, ...)                                            \
    do {                                                                          \
      std::fprintf(stderr, "[%s] %s\n", level,                                    \
                   guandan::util::format(fmt, ##__VA_ARGS__).c_str());            \
    } while (0)
#  define GD_LOG_TRACE(...) GD_LOG_IMPL("TRACE", __VA_ARGS__)
#  define GD_LOG_DEBUG(...) GD_LOG_IMPL("DEBUG", __VA_ARGS__)
#  define GD_LOG_INFO(...)  GD_LOG_IMPL("INFO", __VA_ARGS__)
#  define GD_LOG_WARN(...)  GD_LOG_IMPL("WARN", __VA_ARGS__)
#  define GD_LOG_ERROR(...) GD_LOG_IMPL("ERROR", __VA_ARGS__)
#endif
