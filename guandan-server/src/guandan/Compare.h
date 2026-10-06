#pragma once

#include <cstdint>

#include "guandan/Pattern.h"

namespace guandan::rules {

  enum class CompareResult : uint8_t {
    Incomparable,  // 结构不同，无法比较
    FirstWins,
    SecondWins,
    Tie,
  };

  // 级牌提升后的牌点排序键：大王 > 小王 > 级牌 > A > K > ... > 2。
  int effectiveRank(int rank, int levelRank);

  // 比较两手已识别的牌型（均需有效）。
  CompareResult compare(const PatternInfo& a, const PatternInfo& b, int levelRank);

}  // namespace guandan::rules
