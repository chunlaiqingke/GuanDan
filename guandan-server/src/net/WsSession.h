#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace guandan::net {

  enum class SessionState {
    Handshake,  // 尚未完成 WS 握手
    Open,       // 已握手，可收发帧
    Closing,    // 正在关闭（待 flush 后由 Server 移除）
  };

  // 单个 WS 连接。仅负责 RFC6455 握手 + 帧收发，不关心业务内容。
  class WsSession {
   public:
    // 二进制消息回调：payload 为一条完整 WS 二进制帧内容（即完整业务帧）。
    using BinaryCallback = std::function<void(WsSession&, const uint8_t*, size_t)>;

    explicit WsSession(int fd);
    ~WsSession();

    WsSession(const WsSession&) = delete;
    WsSession& operator=(const WsSession&) = delete;

    int fd() const { return fd_; }
    SessionState state() const { return state_; }
    bool wantWrite() const { return !outBuf_.empty(); }

    // 业务关联（由上层填充）
    int64_t playerId() const { return playerId_; }
    void setPlayerId(int64_t id) { playerId_ = id; }
    const std::string& roomId() const { return roomId_; }
    void setRoomId(const std::string& id) { roomId_ = id; }

    // 心跳（Server 维护）
    void markHeartbeat(int64_t nowMs) {
      lastHeartbeatMs_ = nowMs;
      missedHeartbeats_ = 0;
    }
    int64_t lastHeartbeatMs() const { return lastHeartbeatMs_; }
    int missedHeartbeats() const { return missedHeartbeats_; }
    void incMissedHeartbeat() { ++missedHeartbeats_; }
    void setLastHeartbeatMs(int64_t ms) { lastHeartbeatMs_ = ms; }

    // 追加接收到的字节。
    void appendInput(const uint8_t* data, size_t len);

    // 推进握手/解析帧；返回 false 表示协议错误需关闭。
    bool process(const BinaryCallback& onBinary);

    // 发送二进制业务帧（自动封装 WS 二进制帧，服务端不掩码）。
    void sendBinary(const std::vector<uint8_t>& payload);

    // 尝试写出发送缓冲，处理 EAGAIN。
    void flush();

    // 发送 close 帧并标记 Closing。
    void close();

   private:
    bool tryHandshake();
    void writeRaw(const uint8_t* data, size_t len) {
      outBuf_.insert(outBuf_.end(), data, data + len);
    }

    int fd_;
    SessionState state_ = SessionState::Handshake;
    std::vector<uint8_t> inBuf_;
    std::vector<uint8_t> outBuf_;
    size_t outOff_ = 0;

    int64_t playerId_ = 0;
    std::string roomId_;

    int64_t lastHeartbeatMs_ = 0;
    int missedHeartbeats_ = 0;
  };

}  // namespace guandan::net
