#include "guandan/ai/Strategy.h"

#include <algorithm>

#include "guandan/Compare.h"

namespace guandan::ai {

  namespace {

    // 领出：选有效点最小的单张（保住级牌/王）。
    Play leadPlay(const std::vector<Play>& plays, int levelRank) {
      const Play* best = nullptr;
      for (const auto& p : plays) {
        if (p.pattern.type != rules::PatternType::Single) continue;
        if (!best ||
            rules::effectiveRank(p.pattern.mainRank, levelRank) <
                rules::effectiveRank(best->pattern.mainRank, levelRank)) {
          best = &p;
        }
      }
      return best ? *best : plays.front();
    }

  }  // namespace

  Decision decide(const std::vector<Card>& hand, int levelRank,
                  const rules::PatternInfo* lastPlay, const PlayContext& ctx,
                  Difficulty diff) {
    Decision d;
    if (hand.empty()) {
      d.pass = true;
      return d;
    }

    const auto plays = generatePlays(hand, levelRank);
    if (plays.empty()) {
      d.pass = true;
      return d;
    }

    // 1. 能走完优先（跟牌时还需能压过上一手）
    for (const auto& p : plays) {
      if (p.cards.size() == hand.size()) {
        if (lastPlay == nullptr ||
            rules::compare(p.pattern, *lastPlay, levelRank) == rules::CompareResult::FirstWins) {
          d.cards = p.cards;
          d.tag = ReasonTag::GoOut;
          return d;
        }
      }
    }

    if (lastPlay == nullptr) {
      d.cards = leadPlay(plays, levelRank).cards;
      d.tag = ReasonTag::None;
      return d;
    }

    if (diff == Difficulty::Easy) {
      if (ctx.canPass) {
        d.pass = true;
        d.tag = ReasonTag::LetTeammate;
      } else if (auto mb = minBeat(hand, levelRank, *lastPlay)) {
        d.cards = mb->cards;
        d.tag = ReasonTag::MinBeat;
      } else {
        d.pass = true;
      }
      return d;
    }

    // 普通/困难：启发式
    if (ctx.canPass && ctx.lastPlayIsTeammate) {
      d.pass = true;
      d.tag = ReasonTag::LetTeammate;
      return d;
    }

    auto mb = minBeat(hand, levelRank, *lastPlay);
    if (!mb) {
      d.pass = true;
      d.tag = ReasonTag::LetTeammate;
      return d;
    }

    const bool needBomb = rules::isBombLevel(mb->pattern.type) && !rules::isBombLevel(lastPlay->type);
    if (needBomb) {
      if (ctx.minOpponentHand > 3) {
        d.pass = true;
        d.tag = ReasonTag::SaveBomb;
        return d;
      }
      d.cards = mb->cards;
      d.tag = ReasonTag::BlockOpp;
      return d;
    }

    d.cards = mb->cards;
    d.tag = ReasonTag::MinBeat;
    return d;
  }

  std::vector<Play> hint(const std::vector<Card>& hand, int levelRank,
                         const rules::PatternInfo* lastPlay) {
    auto plays = generatePlays(hand, levelRank);
    if (lastPlay) {
      plays.erase(std::remove_if(plays.begin(), plays.end(),
                                 [&](const Play& p) {
                                   return rules::compare(p.pattern, *lastPlay, levelRank) !=
                                          rules::CompareResult::FirstWins;
                                 }),
                  plays.end());
    }
    std::sort(plays.begin(), plays.end(), [&](const Play& a, const Play& b) {
      const bool aOut = a.cards.size() == hand.size();
      const bool bOut = b.cards.size() == hand.size();
      if (aOut != bOut) return aOut;  // 能走完优先
      const bool ab = rules::isBombLevel(a.pattern.type);
      const bool bb = rules::isBombLevel(b.pattern.type);
      if (ab != bb) return !ab;  // 非炸弹优先
      const int ae = rules::effectiveRank(a.pattern.mainRank, levelRank);
      const int be = rules::effectiveRank(b.pattern.mainRank, levelRank);
      if (ae != be) return ae < be;
      return a.cards.size() < b.cards.size();
    });
    if (plays.size() > 3) plays.resize(3);
    return plays;
  }

}  // namespace guandan::ai
