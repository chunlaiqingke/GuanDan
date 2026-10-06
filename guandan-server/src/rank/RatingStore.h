#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

struct sqlite3;

namespace guandan::rank {

  // SQLite 持久化 playerId -> rating。
  class RatingStore {
   public:
    RatingStore() = default;
    ~RatingStore();
    RatingStore(const RatingStore&) = delete;
    RatingStore& operator=(const RatingStore&) = delete;

    bool open(const std::string& path);
    int getRating(int64_t playerId) const;
    void setRating(int64_t playerId, int rating);
    std::unordered_map<int64_t, int> all() const;

   private:
    sqlite3* db_ = nullptr;
  };

}  // namespace guandan::rank
