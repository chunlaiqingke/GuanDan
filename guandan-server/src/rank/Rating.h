#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace guandan::rank {

  constexpr int kDefaultRating = 1200;
  constexpr int kDefaultK = 32;

  struct RatingUpdate {
    int64_t playerId = 0;
    int oldRating = kDefaultRating;
    int newRating = kDefaultRating;
    int delta = 0;
  };

  // 2v2 团队 ELO：a 队两个 (playerId, rating)、b 队两个；teamAWins 表示 a 队胜。
  std::vector<RatingUpdate> computeRatings(std::pair<int64_t, int> a1,
                                           std::pair<int64_t, int> a2,
                                           std::pair<int64_t, int> b1,
                                           std::pair<int64_t, int> b2,
                                           bool teamAWins, int k = kDefaultK);

  // 段位名称。
  std::string tierName(int rating);

}  // namespace guandan::rank
