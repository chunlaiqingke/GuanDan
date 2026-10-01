#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "net/WsSession.h"
#include "room/RoomManager.h"

namespace guandan::proto {

  // 业务帧分发：解析 [cmd][ver][len][body]，调用对应处理器。
  class Dispatcher {
   public:
    explicit Dispatcher(room::RoomManager& rooms);

    // 处理一条完整业务帧（即 WS 二进制 payload）。
    void onBinary(net::WsSession& s, const uint8_t* data, size_t len);
    void onDisconnect(net::WsSession& s);

   private:
    void sendError(net::WsSession& s, int code, const std::string& msg);

    void handleLogin(net::WsSession& s, const std::string& body);
    void handleCreateRoom(net::WsSession& s, const std::string& body);
    void handleJoinRoom(net::WsSession& s, const std::string& body);
    void handleHeartbeat(net::WsSession& s, const std::string& body);
    void handleReconnect(net::WsSession& s, const std::string& body);

    room::RoomManager& rooms_;
    int64_t nextPlayerId_ = 1;
  };

}  // namespace guandan::proto
