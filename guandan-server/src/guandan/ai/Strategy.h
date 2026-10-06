#pragma once

#include <vector>

#include "guandan/Card.h"
#include "guandan/Pattern.h"
#include "guandan/ai/PlayGen.h"

namespace guandan::ai {

  enum class Difficulty : uint8_t { Easy, Normal, Hard };

  // 出牌理由标签（Phase 05 教学用，先定义）。
  enum class ReasonTag : uint8_t {
    None = 0,
    GoOut,        // 能走完
    MinBeat,      // 最小压牌
    BeatLast,     // 压牌
    SaveBomb,     // 保留炸弹
    LetTeammate,  // 让牌给队友
    BlockOpp,     // 堵对手
  };

  struct PlayContext {
    bool canPass = false;
    bool lastPlayIsTeammate = false;  // 上一手是队友的（可让）
    int minOpponentHand = 99;         // 对手最少手牌数
  };

  struct Decision {
    bool pass = false;
    std::vector<Card> cards;
    ReasonTag tag = ReasonTag::None;
  };

  // 决策：lastPlay=nullptr 表示领出。
  Decision decide(const std::vector<Card>& hand, int levelRank,
                  const rules::PatternInfo* lastPlay, const PlayContext& ctx,
                  Difficulty diff);

  // 提示：返回 Top3 候选（已按策略偏好排序）。lastPlay=nullptr 表示领出。
  std::vector<Play> hint(const std::vector<Card>& hand, int levelRank,
                         const rules::PatternInfo* lastPlay);

}  // namespace guandan::ai
