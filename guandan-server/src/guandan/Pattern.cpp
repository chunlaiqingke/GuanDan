#include "guandan/Pattern.h"

#include <algorithm>

namespace guandan::rules {

  namespace {

    constexpr int kMinSeqRank = 3;  // 顺子/连对/飞机不含 2 与王

    bool isConsecutive(const std::vector<int>& ranks) {
      for (size_t i = 1; i < ranks.size(); ++i) {
        if (ranks[i] != ranks[i - 1] + 1) return false;
      }
      return true;
    }

    // 无逢人配的核心识别。
    PatternInfo classifyCore(const std::vector<Card>& cards) {
      const int n = static_cast<int>(cards.size());
      PatternInfo info;
      if (n == 0) return info;

      int bigJoker = 0;
      int smallJoker = 0;
      int cnt[18] = {0};  // 索引 = 牌点（2..17）
      bool sameSuit = true;
      Suit firstSuit = Suit::Spade;
      bool hasFirst = false;

      for (Card c : cards) {
        const int r = rankOf(c);
        if (r == RANK_BIG_JOKER) {
          ++bigJoker;
        } else if (r == RANK_SMALL_JOKER) {
          ++smallJoker;
        } else {
          ++cnt[r];
          const Suit s = suitOf(c);
          if (!hasFirst) {
            firstSuit = s;
            hasFirst = true;
          } else if (s != firstSuit) {
            sameSuit = false;
          }
        }
      }

      const int jokers = bigJoker + smallJoker;

      // ---- 含王的组合 ----
      if (jokers > 0) {
        if (jokers != n) return info;  // 王与非王混搭非法
        if (n == 1) {
          info.type = PatternType::Single;
          info.mainRank = (bigJoker == 1) ? RANK_BIG_JOKER : RANK_SMALL_JOKER;
          info.length = 1;
          return info;
        }
        if (n == 2 && (bigJoker == 2 || smallJoker == 2)) {
          info.type = PatternType::Pair;
          info.mainRank = (bigJoker == 2) ? RANK_BIG_JOKER : RANK_SMALL_JOKER;
          info.length = 2;
          return info;
        }
        if (n == 4 && bigJoker == 2 && smallJoker == 2) {
          info.type = PatternType::Rocket;
          info.length = 4;
          return info;
        }
        return info;  // 其余王组合非法
      }

      // ---- 无王：收集出现牌点 ----
      std::vector<int> ranks;
      for (int r = kMinRank; r <= kMaxRank; ++r) {
        if (cnt[r] > 0) ranks.push_back(r);
      }

      // 炸弹（≥4 张同点）
      if (ranks.size() == 1 && n >= 4) {
        info.type = PatternType::Bomb;
        info.mainRank = ranks[0];
        info.length = n;
        return info;
      }

      // 单张 / 对子 / 三张
      if (n == 1) {
        info.type = PatternType::Single;
        info.mainRank = ranks[0];
        info.length = 1;
        return info;
      }
      if (n == 2 && ranks.size() == 1) {
        info.type = PatternType::Pair;
        info.mainRank = ranks[0];
        info.length = 2;
        return info;
      }
      if (n == 3 && ranks.size() == 1) {
        info.type = PatternType::Triple;
        info.mainRank = ranks[0];
        info.length = 3;
        return info;
      }

      // 三带二
      if (n == 5 && ranks.size() == 2) {
        const int a = ranks[0];
        const int b = ranks[1];
        if (cnt[a] == 3 && cnt[b] == 2) {
          info.type = PatternType::TripleWithPair;
          info.mainRank = a;
          info.pairRank = b;
          info.length = 5;
          return info;
        }
        if (cnt[a] == 2 && cnt[b] == 3) {
          info.type = PatternType::TripleWithPair;
          info.mainRank = b;
          info.pairRank = a;
          info.length = 5;
          return info;
        }
        return info;
      }

      // 同花顺（固定 5 张，同花连续，不含 2/王）
      if (n == 5 && ranks.size() == 5 && sameSuit && isConsecutive(ranks) &&
          ranks.front() >= kMinSeqRank) {
        info.type = PatternType::StraightFlush;
        info.mainRank = ranks.back();
        info.length = 5;
        return info;
      }

      // 顺子（固定 5 张，混合花色）
      if (n == 5 && ranks.size() == 5 && isConsecutive(ranks) && ranks.front() >= kMinSeqRank) {
        info.type = PatternType::Straight;
        info.mainRank = ranks.back();
        info.length = 5;
        return info;
      }

      // 连对（固定 3 对 = 6 张）
      if (n == 6 && ranks.size() == 3 && isConsecutive(ranks) && ranks.front() >= kMinSeqRank) {
        bool allPairs = true;
        for (int r : ranks) {
          if (cnt[r] != 2) {
            allPairs = false;
            break;
          }
        }
        if (allPairs) {
          info.type = PatternType::PairSequence;
          info.mainRank = ranks.back();
          info.length = 6;
          return info;
        }
      }

      // 飞机（固定 2 组三张 = 6 张）
      if (n == 6 && ranks.size() == 2 && isConsecutive(ranks) && ranks.front() >= kMinSeqRank) {
        if (cnt[ranks[0]] == 3 && cnt[ranks[1]] == 3) {
          info.type = PatternType::TripleSequence;
          info.mainRank = ranks.back();
          info.length = 6;
          return info;
        }
      }

      return info;
    }

    // 逢人配枚举：每张逢人配可配成 2..14 任意点（花色仍为红桃）。
    void collectWithWildcards(const std::vector<Card>& base,
                              const std::vector<Card>& wildCards,
                              std::vector<PatternInfo>& out) {
      const size_t w = wildCards.size();
      std::vector<int> idx(w, 0);
      const int options = kMaxRank - kMinRank + 1;
      while (true) {
        std::vector<Card> eff = base;
        for (size_t i = 0; i < w; ++i) {
          eff.push_back(makeCard(kMinRank + idx[i], Suit::Heart));
        }
        const PatternInfo p = classifyCore(eff);
        if (p.type != PatternType::Invalid) out.push_back(p);

        size_t i = 0;
        while (i < w) {
          if (++idx[i] < options) break;
          idx[i] = 0;
          ++i;
        }
        if (i == w) break;
      }
    }

    // 弱序比较：炸弹级 > 非炸弹级，再比主牌点、张数。
    bool weaker(const PatternInfo& a, const PatternInfo& b) {
      const bool ab = isBombLevel(a.type);
      const bool bb = isBombLevel(b.type);
      if (ab != bb) return !ab;
      if (a.mainRank != b.mainRank) return a.mainRank < b.mainRank;
      return a.length < b.length;
    }

  }  // namespace

  PatternInfo classify(const std::vector<Card>& cards, int levelRank) {
    if (cards.empty()) return {};

    std::vector<Card> base;
    std::vector<Card> wildCards;
    for (Card c : cards) {
      if (isWildcard(c, levelRank)) wildCards.push_back(c);
      else base.push_back(c);
    }

    if (wildCards.empty()) return classifyCore(base);
    if (base.empty()) return classifyCore(cards);  // 纯逢人配：按级牌本身点识别

    std::vector<PatternInfo> valid;
    collectWithWildcards(base, wildCards, valid);
    if (valid.empty()) return {};

    return *std::max_element(valid.begin(), valid.end(), weaker);
  }

}  // namespace guandan::rules

