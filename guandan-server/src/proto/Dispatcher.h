#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "net/WsSession.h"
#include "room/RoomManager.h"

namespace guandan::proto {

  // 业务帧分发：解析 [cmd][ver][len][body]，调用对应处理器。
  // Phase 03 起同时承担对战编排：开局/出牌/过牌/倒计时/托管/自动出牌。
  class Dispatcher {
   public:
    explicit Dispatcher(room::RoomManager& rooms);

    // 处理一条完整业务帧（即 WS 二进制 payload）。
    void onBinary(net::WsSession& s, const uint8_t* data, size_t len);
    void onDisconnect(net::WsSession& s);
    // 定时 tick（由 WsServer 事件循环驱动）：倒计时/超时自动/托管代出。
    void onTick(int64_t nowMs);

   private:
    void sendError(net::WsSession& s, int code, const std::string& msg);
    void sendTo(int64_t playerId, uint16_t cmd, const std::string& body);
    void broadcastRoom(const room::Room& room, uint16_t cmd, const std::string& body);

    void handleLogin(net::WsSession& s, const std::string& body);
    void handleCreateRoom(net::WsSession& s, const std::string& body);
    void handleJoinRoom(net::WsSession& s, const std::string& body);
    void handleHeartbeat(net::WsSession& s, const std::string& body);
    void handleReconnect(net::WsSession& s, const std::string& body);
    void handlePlay(net::WsSession& s, const std::string& body);
    void handlePass(net::WsSession& s, const std::string& body);
    void handleSetHost(net::WsSession& s, const std::string& body);
    void handleAddBot(net::WsSession& s, const std::string& body);
    void handleHint(net::WsSession& s, const std::string& body);

    void maybeStartGame(const std::string& roomId);
    void beginTurn(room::Room& room);
    void autoResolve(room::Room& room, int seat, int64_t nowMs);
    void botResolve(room::Room& room, int seat);
    void broadcastAction(room::Room& room, int64_t uid,
                         const std::vector<guandan::rules::Card>& cards, bool pass);
    void afterAction(room::Room& room);
    void broadcastRoundEnd(room::Room& room);
    bool isBotSeat(const room::Room& room, int seat) const;
    int errorCode(room::PlayResult r) const;

    room::RoomManager& rooms_;
    std::unordered_map<int64_t, net::WsSession*> sessions_;
    int64_t nextPlayerId_ = 1;
    int64_t nextBotId_ = -1;
  };

}  // namespace guandan::proto
