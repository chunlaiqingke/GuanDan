#pragma once

#include <chrono>
#include <cstdint>

namespace guandan::util {

  // 单调时钟毫秒（用于心跳/倒计时，不用墙钟）。
  inline int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

}  // namespace guandan::util
