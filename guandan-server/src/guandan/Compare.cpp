#include "guandan/Compare.h"

namespace guandan::rules {

  int effectiveRank(int rank, int levelRank) {
    if (rank == RANK_BIG_JOKER) return 100;
    if (rank == RANK_SMALL_JOKER) return 99;
    if (rank == levelRank) return 98;  // 级牌提升到 A 之上
    return rank;  // 2..14
  }

  CompareResult compare(const PatternInfo& a, const PatternInfo& b, int levelRank) {
    if (a.type == PatternType::Invalid || b.type == PatternType::Invalid) {
      return CompareResult::Incomparable;
    }

    const bool aBomb = isBombLevel(a.type);
    const bool bBomb = isBombLevel(b.type);

    // 炸弹级 vs 非炸弹级
    if (aBomb != bBomb) {
      return aBomb ? CompareResult::FirstWins : CompareResult::SecondWins;
    }

    // 天王炸
    if (a.type == PatternType::Rocket || b.type == PatternType::Rocket) {
      if (a.type == PatternType::Rocket && b.type == PatternType::Rocket) return CompareResult::Tie;
      return a.type == PatternType::Rocket ? CompareResult::FirstWins : CompareResult::SecondWins;
    }

    if (aBomb) {
      // 炸弹级：同花顺按 5 张炸弹处理
      const int aLen = (a.type == PatternType::StraightFlush) ? 5 : a.length;
      const int bLen = (b.type == PatternType::StraightFlush) ? 5 : b.length;
      if (aLen != bLen) return aLen > bLen ? CompareResult::FirstWins : CompareResult::SecondWins;
      // 同张数：炸弹比有效点（级牌提升），同花顺比自然点
      const int ar = (a.type == PatternType::Bomb) ? effectiveRank(a.mainRank, levelRank) : a.mainRank;
      const int br = (b.type == PatternType::Bomb) ? effectiveRank(b.mainRank, levelRank) : b.mainRank;
      if (ar == br) return CompareResult::Tie;
      return ar > br ? CompareResult::FirstWins : CompareResult::SecondWins;
    }

    // 非炸弹：必须同型同长
    if (a.type != b.type) return CompareResult::Incomparable;
    if (a.length != b.length) return CompareResult::Incomparable;

    // 顺子类按自然点，其余（单/对/三/三带二）按有效点
    const int ar = isSequence(a.type) ? a.mainRank : effectiveRank(a.mainRank, levelRank);
    const int br = isSequence(b.type) ? b.mainRank : effectiveRank(b.mainRank, levelRank);
    if (ar == br) return CompareResult::Tie;
    return ar > br ? CompareResult::FirstWins : CompareResult::SecondWins;
  }

}  // namespace guandan::rules
