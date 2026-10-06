#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "net/WsSession.h"

namespace guandan::net {

  // 最小单线程 WS 服务端：poll 事件循环 + RFC6455 握手/帧收发。
  // 抽象出 onBinary/onConnect/onDisconnect/onTick，便于日后无痛替换为 uWebSockets。
  class WsServer {
   public:
    using BinaryCallback = std::function<void(WsSession&, const uint8_t*, size_t)>;
    using ConnectCallback = std::function<void(WsSession&)>;
    using DisconnectCallback = std::function<void(WsSession&)>;
    using TickCallback = std::function<void(int64_t nowMs)>;

    WsServer();
    ~WsServer();

    WsServer(const WsServer&) = delete;
    WsServer& operator=(const WsServer&) = delete;

    bool listen(uint16_t port);
    uint16_t port() const { return port_; }
    void run();
    void stop();

    void setOnBinary(BinaryCallback cb) { onBinary_ = std::move(cb); }
    void setOnConnect(ConnectCallback cb) { onConnect_ = std::move(cb); }
    void setOnDisconnect(DisconnectCallback cb) { onDisconnect_ = std::move(cb); }
    void setOnTick(TickCallback cb) { onTick_ = std::move(cb); }

   private:
    void acceptPending();
    void prune();
    void sweepHeartbeats(int64_t nowMs);

    int listenFd_ = -1;
    uint16_t port_ = 0;
    int wakeup_[2] = {-1, -1};
    std::atomic<bool> running_{false};

    std::unordered_map<int, std::unique_ptr<WsSession>> sessions_;

    BinaryCallback onBinary_;
    ConnectCallback onConnect_;
    DisconnectCallback onDisconnect_;
    TickCallback onTick_;

    static constexpr int64_t kHeartbeatIntervalMs = 5000;
    static constexpr int kMaxMissedHeartbeats = 3;
  };

}  // namespace guandan::net
