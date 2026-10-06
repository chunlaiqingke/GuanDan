#include "rank/Rating.h"

#include <cmath>

namespace guandan::rank {

  namespace {

    double expected(double ra, double rb) {
      return 1.0 / (1.0 + std::pow(10.0, (rb - ra) / 400.0));
    }

  }  // namespace

  std::vector<RatingUpdate> computeRatings(std::pair<int64_t, int> a1,
                                           std::pair<int64_t, int> a2,
                                           std::pair<int64_t, int> b1,
                                           std::pair<int64_t, int> b2,
                                           bool teamAWins, int k) {
    const double ra = (a1.second + a2.second) / 2.0;
    const double rb = (b1.second + b2.second) / 2.0;
    const double sa = teamAWins ? 1.0 : 0.0;
    const int delta = static_cast<int>(std::lround(k * (sa - expected(ra, rb))));

    return {
        {a1.first, a1.second, a1.second + delta, delta},
        {a2.first, a2.second, a2.second + delta, delta},
        {b1.first, b1.second, b1.second - delta, -delta},
        {b2.first, b2.second, b2.second - delta, -delta},
    };
  }

  std::string tierName(int rating) {
    if (rating >= 2000) return "王者";
    if (rating >= 1800) return "钻石";
    if (rating >= 1600) return "铂金";
    if (rating >= 1400) return "黄金";
    if (rating >= 1200) return "白银";
    return "青铜";
  }

}  // namespace guandan::rank
