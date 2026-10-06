#include <vector>

#include "guandan/Card.h"
#include "guandan/Pattern.h"
#include "guandan/ai/PlayGen.h"
#include "guandan/ai/Strategy.h"
#include "minitest.h"

using namespace guandan::ai;
using guandan::rules::Card;
using guandan::rules::makeCard;
using guandan::rules::PatternInfo;
using guandan::rules::PatternType;
using guandan::rules::RANK_BIG_JOKER;
using guandan::rules::RANK_SMALL_JOKER;
using guandan::rules::Suit;

namespace {

  Card S(int r) { return makeCard(r, Suit::Spade); }
  Card H(int r) { return makeCard(r, Suit::Heart); }
  Card C(int r) { return makeCard(r, Suit::Club); }
  Card D(int r) { return makeCard(r, Suit::Diamond); }
  Card SJ() { return makeCard(RANK_SMALL_JOKER, Suit::Spade); }
  Card BJ() { return makeCard(RANK_BIG_JOKER, Suit::Spade); }

}  // namespace

TEST(GeneratePlaysBasic) {
  std::vector<Card> hand = {S(3), S(3), S(4), H(5), C(6)};
  const auto plays = generatePlays(hand, 2);
  bool hasSingle3 = false, hasPair3 = false, hasSingle6 = false;
  for (const auto& p : plays) {
    if (p.pattern.type == PatternType::Single && p.pattern.mainRank == 3) hasSingle3 = true;
    if (p.pattern.type == PatternType::Pair && p.pattern.mainRank == 3) hasPair3 = true;
    if (p.pattern.type == PatternType::Single && p.pattern.mainRank == 6) hasSingle6 = true;
  }
  EXPECT_TRUE(hasSingle3);
  EXPECT_TRUE(hasPair3);
  EXPECT_TRUE(hasSingle6);
}

TEST(GeneratePlaysStraight) {
  std::vector<Card> hand = {S(3), S(4), H(5), C(6), D(7)};
  bool hasStraight = false;
  for (const auto& p : generatePlays(hand, 2)) {
    if (p.pattern.type == PatternType::Straight && p.pattern.mainRank == 7) hasStraight = true;
  }
  EXPECT_TRUE(hasStraight);
}

TEST(GeneratePlaysWildcard) {
  // 5h 逢人配 + 两个 8 -> 三张 8
  std::vector<Card> hand = {H(5), S(8), S(8)};
  bool hasTriple8 = false;
  for (const auto& p : generatePlays(hand, 5)) {
    if (p.pattern.type == PatternType::Triple && p.pattern.mainRank == 8) hasTriple8 = true;
  }
  EXPECT_TRUE(hasTriple8);
}

TEST(MinBeatBasic) {
  std::vector<Card> hand = {S(4), S(9), S(9), S(10)};
  PatternInfo last;
  last.type = PatternType::Single;
  last.mainRank = 5;
  last.length = 1;
  const auto mb = minBeat(hand, 2, last);
  EXPECT_TRUE(mb.has_value());
  EXPECT_TRUE(mb->pattern.type == PatternType::Single);
  EXPECT_EQ(mb->pattern.mainRank, 9);
}

TEST(DecideGoOut) {
  std::vector<Card> hand = {S(3), S(3)};
  PlayContext ctx;
  const auto d = decide(hand, 2, nullptr, ctx, Difficulty::Normal);
  EXPECT_TRUE(!d.pass);
  EXPECT_TRUE(d.tag == ReasonTag::GoOut);
  EXPECT_EQ(d.cards.size(), 2u);
}

TEST(DecideLeadSmallest) {
  std::vector<Card> hand = {S(9), S(3), S(2)};
  PlayContext ctx;
  const auto d = decide(hand, 5, nullptr, ctx, Difficulty::Normal);  // 打 5，2 非级牌
  EXPECT_TRUE(!d.pass);
  EXPECT_EQ(d.cards.size(), 1u);
  EXPECT_EQ(d.cards[0], S(2));  // 有效点最小
}

TEST(DecideSaveBomb) {
  std::vector<Card> hand = {S(2), H(2), C(2), D(2), S(3)};  // 炸弹 + 小单
  PatternInfo last;
  last.type = PatternType::Single;
  last.mainRank = 10;
  last.length = 1;
  PlayContext ctx;
  ctx.canPass = true;
  ctx.minOpponentHand = 10;  // 对手牌还多，保炸弹
  const auto d = decide(hand, 5, &last, ctx, Difficulty::Normal);
  EXPECT_TRUE(d.pass);
  EXPECT_TRUE(d.tag == ReasonTag::SaveBomb);
}

TEST(DecideMinBeat) {
  std::vector<Card> hand = {S(4), S(9)};
  PatternInfo last;
  last.type = PatternType::Single;
  last.mainRank = 5;
  last.length = 1;
  PlayContext ctx;
  ctx.canPass = true;
  ctx.minOpponentHand = 10;
  const auto d = decide(hand, 2, &last, ctx, Difficulty::Normal);
  EXPECT_TRUE(!d.pass);
  EXPECT_TRUE(d.tag == ReasonTag::MinBeat);
  EXPECT_EQ(d.cards.size(), 1u);
}

TEST(HintTop3) {
  std::vector<Card> hand = {S(3), S(4), S(9), S(10)};
  const auto h = hint(hand, 2, nullptr);
  EXPECT_TRUE(h.size() >= 1);
  EXPECT_TRUE(h.size() <= 3);
  EXPECT_TRUE(h[0].pattern.type == PatternType::Single);
  EXPECT_EQ(h[0].pattern.mainRank, 3);
}

MINITEST_MAIN()
