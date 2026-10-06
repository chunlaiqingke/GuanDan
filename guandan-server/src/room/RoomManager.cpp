#include "room/RoomManager.h"

#include <string>

#include "util/Random.h"

namespace guandan::room {

  std::string RoomManager::genRoomId() {
    while (true) {
      const std::string id = std::to_string(100000 + util::randRange(0, 899999));
      if (rooms_.find(id) == rooms_.end()) return id;
    }
  }

  std::string RoomManager::createRoom(int64_t hostPlayerId) {
    const std::string id = genRoomId();
    Room room;
    room.roomId = id;
    room.hostPlayerId = hostPlayerId;
    rooms_[id] = std::move(room);
    return id;
  }

  bool RoomManager::joinRoom(const std::string& roomId, int64_t playerId) {
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) return false;
    Room& room = it->second;
    // 先判重复（幂等），再判满员：已在本房间的玩家重复加入应视为成功。
    for (const auto& p : room.players) {
      if (p.playerId == playerId) return true;
    }
    if (room.players.size() >= kMaxPlayers) return false;
    RoomPlayer p;
    p.playerId = playerId;
    p.seat = static_cast<int32_t>(room.players.size());
    room.players.push_back(p);
    return true;
  }

  bool RoomManager::exists(const std::string& roomId) const {
    return rooms_.find(roomId) != rooms_.end();
  }

  const Room* RoomManager::find(const std::string& roomId) const {
    auto it = rooms_.find(roomId);
    return it == rooms_.end() ? nullptr : &it->second;
  }

  Room* RoomManager::findMutable(const std::string& roomId) {
    auto it = rooms_.find(roomId);
    return it == rooms_.end() ? nullptr : &it->second;
  }

  void RoomManager::forEachRoom(const std::function<void(Room&)>& fn) {
    for (auto& [id, room] : rooms_) fn(room);
  }

}  // namespace guandan::room
