#pragma once

#include <optional>
#include <vector>

#include "guandan/Card.h"
#include "guandan/Pattern.h"

namespace guandan::ai {

  using rules::Card;

  struct Play {
    std::vector<Card> cards;
    rules::PatternInfo pattern;
  };

  // 枚举一手牌可出的候选牌型（含逢人配配牌）。
  std::vector<Play> generatePlays(const std::vector<Card>& hand, int levelRank);

  // 找最小能压过 lastPlay 的一手；找不到返回 nullopt。
  std::optional<Play> minBeat(const std::vector<Card>& hand, int levelRank,
                              const rules::PatternInfo& lastPlay);

}  // namespace guandan::ai
