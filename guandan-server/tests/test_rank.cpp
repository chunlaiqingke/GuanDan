#include <unordered_map>

#include "minitest.h"
#include "rank/Rating.h"
#include "rank/RatingStore.h"

using namespace guandan::rank;

TEST(ComputeRatingsWin) {
  // 势均力敌：a 队胜，各 +16，b 队 -16（K=32，期望 0.5）
  const auto ups = computeRatings({1, 1200}, {2, 1200}, {3, 1200}, {4, 1200}, true);
  EXPECT_EQ(ups.size(), 4u);
  EXPECT_EQ(ups[0].delta, 16);
  EXPECT_EQ(ups[2].delta, -16);
}

TEST(ComputeRatingsUpset) {
  // 强队(1800)胜弱队(1000)：奖励少于势均力敌
  const auto ups = computeRatings({1, 1800}, {2, 1800}, {3, 1000}, {4, 1000}, true);
  EXPECT_TRUE(ups[0].delta < 16);
  EXPECT_TRUE(ups[0].delta >= 0);
}

TEST(TierName) {
  EXPECT_STREQ(tierName(1000), "青铜");
  EXPECT_STREQ(tierName(1200), "白银");
  EXPECT_STREQ(tierName(1400), "黄金");
  EXPECT_STREQ(tierName(1600), "铂金");
  EXPECT_STREQ(tierName(1800), "钻石");
  EXPECT_STREQ(tierName(2000), "王者");
}

TEST(RatingStoreRoundTrip) {
  RatingStore store;
  EXPECT_TRUE(store.open(":memory:"));
  EXPECT_EQ(store.getRating(42), 1200);  // 默认
  store.setRating(42, 1350);
  EXPECT_EQ(store.getRating(42), 1350);
  store.setRating(42, 1400);  // upsert
  EXPECT_EQ(store.getRating(42), 1400);
  const auto all = store.all();
  EXPECT_EQ(all.at(42), 1400);
}

MINITEST_MAIN()
