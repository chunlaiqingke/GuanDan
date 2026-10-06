#include <algorithm>
#include <vector>

#include "guandan/Card.h"
#include "guandan/Compare.h"
#include "guandan/Pattern.h"
#include "minitest.h"

using namespace guandan::rules;

namespace {

  Card S(int r) { return makeCard(r, Suit::Spade); }
  Card H(int r) { return makeCard(r, Suit::Heart); }
  Card C(int r) { return makeCard(r, Suit::Club); }
  Card D(int r) { return makeCard(r, Suit::Diamond); }
  Card SJ() { return makeCard(RANK_SMALL_JOKER, Suit::Spade); }
  Card BJ() { return makeCard(RANK_BIG_JOKER, Suit::Spade); }

}  // namespace

TEST(CardEncoding) {
  const Card c = makeCard(RANK_A, Suit::Heart);
  EXPECT_EQ(rankOf(c), RANK_A);
  EXPECT_TRUE(suitOf(c) == Suit::Heart);
  EXPECT_TRUE(!isJoker(c));

  EXPECT_TRUE(isJoker(SJ()));
  EXPECT_TRUE(isSmallJoker(SJ()));
  EXPECT_TRUE(isBigJoker(BJ()));

  EXPECT_TRUE(isWildcard(H(5), 5));
  EXPECT_TRUE(!isWildcard(S(5), 5));
  EXPECT_TRUE(!isWildcard(H(6), 5));
}

TEST(DeckHas108AndCounts) {
  const auto deck = buildDeck();
  EXPECT_EQ(deck.size(), 108u);

  int cnt[18] = {0};
  for (Card c : deck) cnt[rankOf(c)]++;
  for (int r = 2; r <= 14; ++r) EXPECT_EQ(cnt[r], 8);
  EXPECT_EQ(cnt[RANK_SMALL_JOKER], 2);
  EXPECT_EQ(cnt[RANK_BIG_JOKER], 2);
}

TEST(ShuffleDeterministicAndDeal) {
  auto d1 = buildDeck();
  auto d2 = buildDeck();
  shuffle(d1, 123);
  shuffle(d2, 123);
  EXPECT_TRUE(d1 == d2);

  auto d3 = buildDeck();
  shuffle(d3, 999);
  EXPECT_TRUE(d1 != d3);

  const auto hands = deal(d1);
  for (const auto& h : hands) EXPECT_EQ(h.size(), 27u);

  std::vector<Card> all;
  for (const auto& h : hands) all.insert(all.end(), h.begin(), h.end());
  std::sort(all.begin(), all.end());
  auto sorted = d1;
  std::sort(sorted.begin(), sorted.end());
  EXPECT_TRUE(all == sorted);
}

TEST(ClassifyBasicTypes) {
  EXPECT_TRUE(classify({S(3)}, 2).type == PatternType::Single);
  EXPECT_TRUE(classify({S(10), H(10)}, 2).type == PatternType::Pair);
  EXPECT_TRUE(classify({S(7), H(7), C(7)}, 2).type == PatternType::Triple);
  EXPECT_TRUE(classify({S(7), H(7), C(7), S(4), H(4)}, 2).type == PatternType::TripleWithPair);
  EXPECT_TRUE(classify({S(3), H(4), C(5), D(6), S(7)}, 2).type == PatternType::Straight);
  EXPECT_TRUE(classify({S(3), H(3), C(4), D(4), S(5), H(5)}, 2).type == PatternType::PairSequence);
  EXPECT_TRUE(classify({S(8), H(8), C(8), S(9), H(9), C(9)}, 2).type == PatternType::TripleSequence);
  EXPECT_TRUE(classify({H(4), H(5), H(6), H(7), H(8)}, 2).type == PatternType::StraightFlush);
  EXPECT_TRUE(classify({S(9), H(9), C(9), D(9)}, 2).type == PatternType::Bomb);
  EXPECT_TRUE(classify({S(2), H(2), C(2), D(2)}, 2).type == PatternType::Bomb);
  EXPECT_TRUE(classify({SJ(), SJ(), BJ(), BJ()}, 2).type == PatternType::Rocket);
}

TEST(ClassifyInvalid) {
  EXPECT_TRUE(classify({SJ(), BJ()}, 2).type == PatternType::Invalid);  // 一大一小王
  EXPECT_TRUE(classify({S(7), H(7), C(7), D(4)}, 2).type == PatternType::Invalid);  // 3+1
  EXPECT_TRUE(classify({S(2), H(3), C(4), D(5), S(6)}, 2).type == PatternType::Invalid);  // 顺子含 2
  EXPECT_TRUE(classify({SJ(), H(3), C(4), D(5), S(6)}, 2).type == PatternType::Invalid);  // 顺子含王
  EXPECT_TRUE(classify({S(3), H(4), C(5), D(6), S(8)}, 2).type == PatternType::Invalid);  // 非连续
  EXPECT_TRUE(classify({S(3), H(3), C(4), D(4), S(6), H(6)}, 2).type == PatternType::Invalid);  // 连对非连续
  EXPECT_TRUE(classify({S(8), H(8), C(8), S(10), H(10), C(10)}, 2).type == PatternType::Invalid);  // 飞机非连续
}

TEST(ClassifyWildcard) {
  const int level = 5;
  const Card wild = H(5);

  auto p = classify({wild, S(8), H(8)}, level);
  EXPECT_TRUE(p.type == PatternType::Triple);
  EXPECT_EQ(p.mainRank, 8);

  auto s = classify({wild, S(3), S(4), S(6), S(7)}, level);
  EXPECT_TRUE(s.type == PatternType::Straight);
  EXPECT_EQ(s.mainRank, 7);

  auto t = classify({wild, H(5), S(8)}, level);
  EXPECT_TRUE(t.type == PatternType::Triple);
  EXPECT_EQ(t.mainRank, 8);

  EXPECT_TRUE(classify({wild}, level).type == PatternType::Single);
  EXPECT_EQ(classify({wild}, level).mainRank, level);

  auto f = classify({wild, H(7), H(8), H(9), H(10)}, level);
  EXPECT_TRUE(f.type == PatternType::StraightFlush);
  EXPECT_EQ(f.mainRank, RANK_J);
}

TEST(CompareSingles) {
  const int level = 5;
  const auto big = classify({BJ()}, level);
  const auto small = classify({SJ()}, level);
  const auto lv = classify({S(5)}, level);
  const auto a = classify({S(RANK_A)}, level);
  const auto k = classify({S(RANK_K)}, level);
  const auto two = classify({S(2)}, level);

  EXPECT_TRUE(compare(big, small, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(small, lv, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(lv, a, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(a, k, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(k, two, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(a, a, level) == CompareResult::Tie);
}

TEST(CompareBombs) {
  const int level = 2;
  const auto b4 = classify({S(7), H(7), C(7), D(7)}, level);
  const auto b5 = classify({S(7), S(7), H(7), H(7), C(7)}, level);
  const auto b6 = classify({S(7), S(7), H(7), H(7), C(7), D(7)}, level);
  const auto b9 = classify({S(9), H(9), C(9), D(9)}, level);

  EXPECT_TRUE(compare(b5, b4, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(b6, b5, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(b9, b4, level) == CompareResult::FirstWins);

  const auto lvBomb = classify({S(5), H(5), C(5), D(5)}, 5);
  const auto aBomb = classify({S(RANK_A), H(RANK_A), C(RANK_A), D(RANK_A)}, 5);
  EXPECT_TRUE(compare(lvBomb, aBomb, 5) == CompareResult::FirstWins);
}

TEST(CompareStraightFlushAndRocket) {
  const int level = 2;
  const auto flush = classify({H(6), H(7), H(8), H(9), H(10)}, level);
  const auto b4 = classify({S(8), H(8), C(8), D(8)}, level);
  const auto b5 = classify({S(7), S(7), H(7), H(7), C(7)}, level);
  const auto b6 = classify({S(7), S(7), H(7), H(7), C(7), D(7)}, level);
  const auto rocket = classify({SJ(), SJ(), BJ(), BJ()}, level);

  EXPECT_TRUE(compare(flush, b4, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(flush, b5, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(b6, flush, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(rocket, b6, level) == CompareResult::FirstWins);
  EXPECT_TRUE(compare(rocket, rocket, level) == CompareResult::Tie);
}

TEST(CompareNonBomb) {
  const int level = 2;
  const auto pair9 = classify({S(9), H(9)}, level);
  const auto pair7 = classify({S(7), H(7)}, level);
  EXPECT_TRUE(compare(pair9, pair7, level) == CompareResult::FirstWins);

  const auto s34567 = classify({S(3), H(4), C(5), D(6), S(7)}, level);
  const auto s45678 = classify({S(4), H(5), C(6), D(7), S(8)}, level);
  EXPECT_TRUE(compare(s45678, s34567, level) == CompareResult::FirstWins);

  const auto triple = classify({S(9), H(9), C(9)}, level);
  EXPECT_TRUE(compare(pair9, triple, level) == CompareResult::Incomparable);

  const auto f77744 = classify({S(7), H(7), C(7), S(4), H(4)}, level);
  const auto f666AA = classify({S(6), H(6), C(6), S(RANK_A), H(RANK_A)}, level);
  EXPECT_TRUE(compare(f77744, f666AA, level) == CompareResult::FirstWins);
}

TEST(StraightUsesNaturalRank) {
  const int level = RANK_J;
  const auto sJ = classify({S(7), H(8), C(9), D(10), S(RANK_J)}, level);
  const auto sQ = classify({S(8), H(9), C(10), D(RANK_J), S(RANK_Q)}, level);
  EXPECT_TRUE(compare(sQ, sJ, level) == CompareResult::FirstWins);
}

MINITEST_MAIN()

