#include "rank/RatingStore.h"

#include <sqlite3.h>

#include <cstdlib>

#include "rank/Rating.h"

namespace guandan::rank {

  RatingStore::~RatingStore() {
    if (db_) sqlite3_close(db_);
  }

  bool RatingStore::open(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) return false;
    const char* sql =
        "CREATE TABLE IF NOT EXISTS ratings(player_id INTEGER PRIMARY KEY, rating INTEGER NOT NULL);";
    return sqlite3_exec(db_, sql, nullptr, nullptr, nullptr) == SQLITE_OK;
  }

  int RatingStore::getRating(int64_t playerId) const {
    int rating = kDefaultRating;
    if (!db_) return rating;
    const std::string sql =
        "SELECT rating FROM ratings WHERE player_id = " + std::to_string(playerId) + ";";
    sqlite3_exec(db_, sql.c_str(),
                 [](void* ctx, int, char** cols, char**) -> int {
                   *static_cast<int*>(ctx) = std::atoi(cols[0]);
                   return 0;
                 },
                 &rating, nullptr);
    return rating;
  }

  void RatingStore::setRating(int64_t playerId, int rating) {
    if (!db_) return;
    const std::string sql = "INSERT INTO ratings(player_id, rating) VALUES(" +
                            std::to_string(playerId) + "," + std::to_string(rating) +
                            ") ON CONFLICT(player_id) DO UPDATE SET rating = excluded.rating;";
    sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
  }

  std::unordered_map<int64_t, int> RatingStore::all() const {
    std::unordered_map<int64_t, int> out;
    if (!db_) return out;
    sqlite3_exec(db_, "SELECT player_id, rating FROM ratings;",
                 [](void* ctx, int, char** cols, char**) -> int {
                   auto* m = static_cast<std::unordered_map<int64_t, int>*>(ctx);
                   (*m)[std::atoll(cols[0])] = std::atoi(cols[1]);
                   return 0;
                 },
                 &out, nullptr);
    return out;
  }

}  // namespace guandan::rank
