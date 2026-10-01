#pragma once

#include <cstdint>

namespace guandan::util {

  // 轻量 xorshift32，Phase 01 用于生成房间号，避免引入重量级随机库。
  inline uint32_t& randState() {
    static uint32_t s = 0x9e3779b9u;
    return s;
  }

  inline void seedRand(uint32_t seed) { randState() = seed; }

  inline uint32_t nextRand() {
    uint32_t& x = randState();
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
  }

  // [lo, hi] 闭区间
  inline uint32_t randRange(uint32_t lo, uint32_t hi) {
    if (hi <= lo) return lo;
    return lo + (nextRand() % (hi - lo + 1));
  }

}  // namespace guandan::util
