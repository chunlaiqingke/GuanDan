#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace guandan::room {

  struct RoomPlayer {
    int64_t playerId = 0;
    std::string name;
    int32_t seat = 0;
  };

  struct Room {
    std::string roomId;
    std::vector<RoomPlayer> players;
    int64_t hostPlayerId = 0;
  };

  // Phase 01 最小房间注册：建房/入房/快照，不含牌局逻辑。
  class RoomManager {
   public:
    std::string createRoom(int64_t hostPlayerId);
    bool joinRoom(const std::string& roomId, int64_t playerId);
    bool exists(const std::string& roomId) const;
    const Room* find(const std::string& roomId) const;

    static constexpr int kMaxPlayers = 4;

   private:
    std::string genRoomId();

    std::unordered_map<std::string, Room> rooms_;
  };

}  // namespace guandan::room
