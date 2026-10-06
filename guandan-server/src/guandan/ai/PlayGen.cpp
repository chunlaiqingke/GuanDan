#include "guandan/ai/PlayGen.h"

#include <algorithm>
#include <set>
#include <utility>
#include <vector>

#include "guandan/Compare.h"

namespace guandan::ai {

  namespace {

    struct Counts {
      int rank[18] = {0};           // 自然牌计数（含王）
      int suitRank[4][18] = {{0}};  // 自然普通牌花色分布
      int wild = 0;                 // 逢人配数量
      int smallJoker = 0;
      int bigJoker = 0;
    };

    Counts buildCounts(const std::vector<Card>& hand, int levelRank) {
      Counts c;
      for (Card card : hand) {
        if (rules::isWildcard(card, levelRank)) {
          ++c.wild;
          continue;
        }
        const int r = rules::rankOf(card);
        ++c.rank[r];
        if (r == rules::RANK_BIG_JOKER) ++c.bigJoker;
        else if (r == rules::RANK_SMALL_JOKER) ++c.smallJoker;
        else if (r >= 2 && r <= 14) ++c.suitRank[static_cast<int>(rules::suitOf(card))][r];
      }
      return c;
    }

    // 取 n 张指定点数的自然牌（不含逢人配）。
    std::vector<Card> findCards(const std::vector<Card>& hand, int levelRank, int rank, int n) {
      std::vector<Card> out;
      for (Card c : hand) {
        if (rules::isWildcard(c, levelRank)) continue;
        if (rules::rankOf(c) != rank) continue;
        out.push_back(c);
        if (static_cast<int>(out.size()) == n) return out;
      }
      return {};
    }

    std::vector<Card> wildcards(const std::vector<Card>& hand, int levelRank) {
      std::vector<Card> out;
      for (Card c : hand) {
        if (rules::isWildcard(c, levelRank)) out.push_back(c);
      }
      return out;
    }

    bool allWildcards(const std::vector<Card>& cards, int levelRank) {
      if (cards.empty()) return false;
      for (Card c : cards) {
        if (!rules::isWildcard(c, levelRank)) return false;
      }
      return true;
    }

    // 从 hand 取牌满足 requirements（rank->数量）；suit>=0 时限同花色。
    // 逢人配作为万能牌补缺（同花顺时仅能参与红桃）。
    std::vector<Card> fulfill(const std::vector<Card>& hand, int levelRank,
                              const std::vector<std::pair<int, int>>& reqs, int suit) {
      std::vector<bool> used(hand.size(), false);
      std::vector<Card> out;
      int total = 0;
      for (const auto& pr : reqs) total += pr.second;

      for (const auto& pr : reqs) {
        int got = 0;
        for (size_t i = 0; i < hand.size() && got < pr.second; ++i) {
          if (used[i] || rules::isWildcard(hand[i], levelRank)) continue;
          if (rules::rankOf(hand[i]) != pr.first) continue;
          if (suit >= 0 && static_cast<int>(rules::suitOf(hand[i])) != suit) continue;
          used[i] = true;
          out.push_back(hand[i]);
          ++got;
        }
      }

      if (static_cast<int>(out.size()) < total) {
        for (size_t i = 0; i < hand.size() && static_cast<int>(out.size()) < total; ++i) {
          if (used[i] || !rules::isWildcard(hand[i], levelRank)) continue;
          if (suit >= 0 && suit != static_cast<int>(rules::Suit::Heart)) continue;
          used[i] = true;
          out.push_back(hand[i]);
        }
      }

      if (static_cast<int>(out.size()) != total) return {};
      return out;
    }

    Play makePlay(const std::vector<Card>& cards, rules::PatternType t, int mainRank, int length,
                  int pairRank = 0) {
      Play p;
      p.cards = cards;
      p.pattern.type = t;
      p.pattern.mainRank = mainRank;
      p.pattern.length = length;
      p.pattern.pairRank = pairRank;
      return p;
    }

  }  // namespace

  std::vector<Play> generatePlays(const std::vector<Card>& hand, int levelRank) {
    std::vector<Play> out;
    if (hand.empty()) return out;

    const Counts c = buildCounts(hand, levelRank);
    const int W = c.wild;
    const auto wcards = wildcards(hand, levelRank);

    auto add = [&](std::vector<Card> cards, rules::PatternType, int, int, int = 0) {
      if (cards.empty()) return;
      if (allWildcards(cards, levelRank)) return;  // 纯逢人配单独处理
      const auto pat = rules::classify(cards, levelRank);  // 以规则引擎为准
      if (pat.type == rules::PatternType::Invalid) return;
      out.push_back(Play{std::move(cards), pat});
    };

    // 纯逢人配：单张/对子按级牌本身。
    if (wcards.size() >= 1) {
      out.push_back(makePlay({wcards[0]}, rules::PatternType::Single, levelRank, 1));
    }
    if (wcards.size() >= 2) {
      out.push_back(makePlay({wcards[0], wcards[1]}, rules::PatternType::Pair, levelRank, 2));
    }

    // 单张（自然牌 + 王）
    for (int r = 2; r <= 14; ++r) {
      auto cs = findCards(hand, levelRank, r, 1);
      if (!cs.empty()) out.push_back(makePlay(cs, rules::PatternType::Single, r, 1));
    }
    if (c.smallJoker >= 1) {
      out.push_back(makePlay(findCards(hand, levelRank, rules::RANK_SMALL_JOKER, 1),
                             rules::PatternType::Single, rules::RANK_SMALL_JOKER, 1));
    }
    if (c.bigJoker >= 1) {
      out.push_back(makePlay(findCards(hand, levelRank, rules::RANK_BIG_JOKER, 1),
                             rules::PatternType::Single, rules::RANK_BIG_JOKER, 1));
    }
    // 王对
    if (c.smallJoker >= 2) {
      out.push_back(makePlay(findCards(hand, levelRank, rules::RANK_SMALL_JOKER, 2),
                             rules::PatternType::Pair, rules::RANK_SMALL_JOKER, 2));
    }
    if (c.bigJoker >= 2) {
      out.push_back(makePlay(findCards(hand, levelRank, rules::RANK_BIG_JOKER, 2),
                             rules::PatternType::Pair, rules::RANK_BIG_JOKER, 2));
    }

    // 对子 / 三张 / 炸弹（普通点数）
    for (int r = 2; r <= 14; ++r) {
      if (c.rank[r] + W >= 2) add(fulfill(hand, levelRank, {{r, 2}}, -1), rules::PatternType::Pair, r, 2);
      if (c.rank[r] + W >= 3) add(fulfill(hand, levelRank, {{r, 3}}, -1), rules::PatternType::Triple, r, 3);
      const int maxN = c.rank[r] + W;
      if (maxN >= 4) {
        add(fulfill(hand, levelRank, {{r, 4}}, -1), rules::PatternType::Bomb, r, 4);
        if (maxN > 4) add(fulfill(hand, levelRank, {{r, maxN}}, -1), rules::PatternType::Bomb, r, maxN);
      }
    }

    // 三带二
    for (int a = 2; a <= 14; ++a) {
      for (int b = 2; b <= 14; ++b) {
        if (a == b) continue;
        add(fulfill(hand, levelRank, {{a, 3}, {b, 2}}, -1), rules::PatternType::TripleWithPair, a, 5, b);
      }
    }

    // 顺子（固定 5，3..A）
    for (int r = 3; r + 4 <= 14; ++r) {
      std::vector<std::pair<int, int>> reqs;
      for (int k = 0; k < 5; ++k) reqs.push_back({r + k, 1});
      add(fulfill(hand, levelRank, reqs, -1), rules::PatternType::Straight, r + 4, 5);
    }

    // 连对（固定 3 对）
    for (int r = 3; r + 2 <= 14; ++r) {
      std::vector<std::pair<int, int>> reqs;
      for (int k = 0; k < 3; ++k) reqs.push_back({r + k, 2});
      add(fulfill(hand, levelRank, reqs, -1), rules::PatternType::PairSequence, r + 2, 6);
    }

    // 飞机（固定 2 组三张）
    for (int r = 3; r + 1 <= 14; ++r) {
      add(fulfill(hand, levelRank, {{r, 3}, {r + 1, 3}}, -1), rules::PatternType::TripleSequence, r + 1, 6);
    }

    // 同花顺
    for (int s = 0; s < 4; ++s) {
      for (int r = 3; r + 4 <= 14; ++r) {
        std::vector<std::pair<int, int>> reqs;
        for (int k = 0; k < 5; ++k) reqs.push_back({r + k, 1});
        add(fulfill(hand, levelRank, reqs, s), rules::PatternType::StraightFlush, r + 4, 5);
      }
    }

    // 天王炸
    if (c.smallJoker == 2 && c.bigJoker == 2) {
      auto cs = findCards(hand, levelRank, rules::RANK_SMALL_JOKER, 2);
      auto cb = findCards(hand, levelRank, rules::RANK_BIG_JOKER, 2);
      if (cs.size() == 2 && cb.size() == 2) {
        cs.insert(cs.end(), cb.begin(), cb.end());
        out.push_back(makePlay(cs, rules::PatternType::Rocket, 0, 4));
      }
    }

    // 去重（按牌面排序）
    std::set<std::vector<Card>> seen;
    std::vector<Play> unique;
    for (auto& p : out) {
      auto key = p.cards;
      std::sort(key.begin(), key.end());
      if (seen.insert(key).second) unique.push_back(std::move(p));
    }
    return unique;
  }

  std::optional<Play> minBeat(const std::vector<Card>& hand, int levelRank,
                              const rules::PatternInfo& lastPlay) {
    std::optional<Play> best;
    auto better = [&](const Play& a, const Play& b) {
      const bool ab = rules::isBombLevel(a.pattern.type);
      const bool bb = rules::isBombLevel(b.pattern.type);
      if (ab != bb) return !ab;  // 非炸弹优先（保炸弹）
      const int ae = rules::effectiveRank(a.pattern.mainRank, levelRank);
      const int be = rules::effectiveRank(b.pattern.mainRank, levelRank);
      if (ae != be) return ae < be;
      return a.cards.size() < b.cards.size();
    };
    for (auto& p : generatePlays(hand, levelRank)) {
      if (rules::compare(p.pattern, lastPlay, levelRank) != rules::CompareResult::FirstWins) continue;
      if (!best || better(p, *best)) best = std::move(p);
    }
    return best;
  }

}  // namespace guandan::ai

