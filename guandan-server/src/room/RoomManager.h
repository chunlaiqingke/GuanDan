#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "room/GameTable.h"

namespace guandan::room {

  struct RoomPlayer {
    int64_t playerId = 0;
    std::string name;
    int32_t seat = 0;
    bool isBot = false;
  };

  struct Room {
    std::string roomId;
    std::vector<RoomPlayer> players;
    int64_t hostPlayerId = 0;

    // Phase 03 对战状态
    int level = 2;
    bool started = false;
    GameTable table;
    int64_t deadlineMs = 0;              // 当前回合截止（单调时钟 ms），0=无
    int64_t nextTickMs = 0;              // 下次推 S2C_Tick 的时间
    std::array<bool, 4> hostMode{};      // 按 seat 托管
    std::array<int, 4> timeoutCount{};   // 按 seat 超时次数
  };

  inline int seatOf(const Room& room, int64_t playerId) {
    for (const auto& p : room.players) {
      if (p.playerId == playerId) return p.seat;
    }
    return -1;
  }

  // Phase 01 最小房间注册；Phase 03 扩展对战状态。
  class RoomManager {
   public:
    std::string createRoom(int64_t hostPlayerId);
    bool joinRoom(const std::string& roomId, int64_t playerId);
    bool addBot(const std::string& roomId, int64_t playerId);
    bool exists(const std::string& roomId) const;
    const Room* find(const std::string& roomId) const;
    Room* findMutable(const std::string& roomId);
    void forEachRoom(const std::function<void(Room&)>& fn);

    static constexpr int kMaxPlayers = 4;

   private:
    std::string genRoomId();

    std::unordered_map<std::string, Room> rooms_;
  };

}  // namespace guandan::room
