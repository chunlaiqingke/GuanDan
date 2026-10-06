#include <array>
#include <vector>

#include "guandan/Card.h"
#include "minitest.h"
#include "room/GameTable.h"

using namespace guandan::room;
using guandan::rules::Card;
using guandan::rules::makeCard;
using guandan::rules::Suit;

namespace {

  Card S(int r) { return makeCard(r, Suit::Spade); }
  Card H(int r) { return makeCard(r, Suit::Heart); }
  Card C(int r) { return makeCard(r, Suit::Club); }
  Card D(int r) { return makeCard(r, Suit::Diamond); }

}  // namespace

TEST(StartRoundDeals27) {
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, 12345u);
  EXPECT_TRUE(t.phase() == Phase::Playing);
  EXPECT_EQ(t.currentTurn(), 0);
  int total = 0;
  for (int s = 0; s < 4; ++s) {
    EXPECT_EQ(t.handCount(s), 27);
    total += t.handCount(s);
  }
  EXPECT_EQ(total, 108);
}

TEST(TurnOrderAndValidation) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(3), S(3), S(4), S(5), S(6), S(7), S(8), S(9)};
  hands[1] = {H(10), H(10)};
  hands[2] = {C(9), C(9), C(9)};
  hands[3] = {D(2)};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 5, hands);

  EXPECT_TRUE(t.play(1, {H(10)}) == PlayResult::NotYourTurn);

  EXPECT_TRUE(t.play(0, {S(3)}) == PlayResult::Ok);  // 领出单张 3
  EXPECT_EQ(t.currentTurn(), 1);
  EXPECT_TRUE(t.play(1, {H(10)}) == PlayResult::Ok);  // 单张 10 压过 3
  EXPECT_EQ(t.currentTurn(), 2);
  EXPECT_TRUE(t.play(2, {C(9)}) == PlayResult::CannotBeat);  // 9 压不过 10
  EXPECT_TRUE(t.pass(2) == PlayResult::Ok);
  EXPECT_EQ(t.currentTurn(), 3);
  EXPECT_TRUE(t.play(3, {D(2)}) == PlayResult::CannotBeat);
  EXPECT_TRUE(t.pass(3) == PlayResult::Ok);
  EXPECT_TRUE(t.pass(0) == PlayResult::Ok);  // 3 连过 → 圈结束
  EXPECT_EQ(t.currentTurn(), 1);             // 赢家 seat1 继续
}

TEST(LeadCannotPass) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(3)};
  hands[1] = {S(4)};
  hands[2] = {S(5)};
  hands[3] = {S(6)};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, hands);
  EXPECT_TRUE(t.pass(0) == PlayResult::MustLead);
}

TEST(PlayValidation) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(3), S(3), S(4)};
  hands[1] = {S(5)};
  hands[2] = {S(6)};
  hands[3] = {S(7)};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, hands);

  EXPECT_TRUE(t.play(0, {S(9)}) == PlayResult::CardsNotInHand);
  EXPECT_TRUE(t.play(0, {S(3), S(4)}) == PlayResult::InvalidCards);  // 3+4 不是对子
  EXPECT_TRUE(t.play(0, {S(3), S(3)}) == PlayResult::Ok);            // 对子 3
}

TEST(RoundEndAndLevelUpTwo) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(2), H(2), C(2), D(2)};  // 炸弹 2222
  hands[1] = {S(3)};
  hands[2] = {S(4)};
  hands[3] = {S(5)};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, hands);

  EXPECT_TRUE(t.play(0, {S(2), H(2), C(2), D(2)}) == PlayResult::Ok);  // 头游
  EXPECT_TRUE(t.pass(1) == PlayResult::Ok);
  EXPECT_TRUE(t.pass(2) == PlayResult::Ok);
  EXPECT_TRUE(t.pass(3) == PlayResult::Ok);  // 圈结束 → 接风 seat2
  EXPECT_EQ(t.currentTurn(), 2);
  EXPECT_TRUE(t.play(2, {S(4)}) == PlayResult::Ok);  // 二游
  EXPECT_EQ(t.currentTurn(), 3);
  EXPECT_TRUE(t.play(3, {S(5)}) == PlayResult::Ok);  // 三游 → 结束
  EXPECT_TRUE(t.phase() == Phase::RoundEnd);
  EXPECT_EQ(t.headSeat(), 0);
  EXPECT_EQ(t.lastSeat(), 1);
  EXPECT_EQ(t.finishOrder().size(), 3u);
  EXPECT_EQ(t.finishOrder()[1].seat, 2);
  EXPECT_EQ(t.finishOrder()[2].seat, 3);
  EXPECT_EQ(t.levelUp(), 2);
}

TEST(LevelUpOne) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(3)};
  hands[1] = {S(4)};
  hands[2] = {S(5)};
  hands[3] = {S(2)};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, hands);

  EXPECT_TRUE(t.play(0, {S(3)}) == PlayResult::Ok);  // 头游
  EXPECT_TRUE(t.play(1, {S(4)}) == PlayResult::Ok);  // 二游（对手）
  EXPECT_TRUE(t.play(2, {S(5)}) == PlayResult::Ok);  // 三游
  EXPECT_TRUE(t.phase() == Phase::RoundEnd);
  EXPECT_EQ(t.levelUp(), 1);
  EXPECT_EQ(t.lastSeat(), 3);
}

TEST(ComputeTribute) {
  // 头游 0、二游 2（同队）→ 双下：末游(3)→头游(0)，三游(1)→二游(2)
  std::vector<FinishInfo> fo = {{0, 1}, {2, 2}, {1, 3}};
  auto t = GameTable::computeTribute(fo);
  EXPECT_EQ(t.size(), 2u);
  EXPECT_EQ(t[0].fromSeat, 3);
  EXPECT_EQ(t[0].toSeat, 0);
  EXPECT_EQ(t[1].fromSeat, 1);
  EXPECT_EQ(t[1].toSeat, 2);

  // 头游 0、二游 1（对手）→ 单贡：末游(3)→头游(0)
  std::vector<FinishInfo> fo2 = {{0, 1}, {1, 2}, {2, 3}};
  auto t2 = GameTable::computeTribute(fo2);
  EXPECT_EQ(t2.size(), 1u);
  EXPECT_EQ(t2[0].fromSeat, 3);
  EXPECT_EQ(t2[0].toSeat, 0);
}

TEST(MinSingle) {
  std::array<std::vector<Card>, 4> hands;
  hands[0] = {S(9), S(3), S(2), S(10)};
  hands[1] = {};
  hands[2] = {};
  hands[3] = {};
  GameTable t;
  t.startRound({1, 2, 3, 4}, 2, hands);
  const auto m = t.minSingle(0);
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m[0], S(2));
}

MINITEST_MAIN()

