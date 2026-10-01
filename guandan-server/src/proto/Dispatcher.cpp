#include "proto/Dispatcher.h"

#include <string>

#include "messages.pb.h"
#include "net/Frame.h"
#include "proto/Cmd.h"
#include "util/Log.h"
#include "util/Time.h"

namespace guandan::proto {

Dispatcher::Dispatcher(room::RoomManager& rooms) : rooms_(rooms) {}

void Dispatcher::onBinary(net::WsSession& s, const uint8_t* data, size_t len) {
  net::FrameHeader hdr;
  if (!net::decodeHeader(data, len, hdr)) {
    GD_LOG_WARN("bad frame header, len={}", len);
    sendError(s, 400, "bad frame header");
    return;
  }
  if (hdr.ver != net::kVersion) {
    GD_LOG_WARN("unsupported version {}", hdr.ver);
    sendError(s, 400, "unsupported version");
    return;
  }
  if (hdr.len != len - net::kFrameHeaderSize) {
    GD_LOG_WARN("frame length mismatch hdr={} actual={}", hdr.len, len - net::kFrameHeaderSize);
    sendError(s, 400, "frame length mismatch");
    return;
  }

  const std::string body(reinterpret_cast<const char*>(data + net::kFrameHeaderSize), hdr.len);

  switch (static_cast<Cmd>(hdr.cmd)) {
    case Cmd::C2S_Login:
      handleLogin(s, body);
      break;
    case Cmd::C2S_CreateRoom:
      handleCreateRoom(s, body);
      break;
    case Cmd::C2S_JoinRoom:
      handleJoinRoom(s, body);
      break;
    case Cmd::C2S_Heartbeat:
      handleHeartbeat(s, body);
      break;
    case Cmd::C2S_Reconnect:
      handleReconnect(s, body);
      break;
    default:
      GD_LOG_WARN("unknown cmd {}", hdr.cmd);
      sendError(s, 404, "unknown cmd");
      break;
  }
}

void Dispatcher::onDisconnect(net::WsSession& s) {
  GD_LOG_INFO("disconnect playerId={} roomId={}", s.playerId(), s.roomId());
  // Phase 01：不处理退房，后续 Phase 03 补齐。
}

void Dispatcher::sendError(net::WsSession& s, int code, const std::string& msg) {
  ErrorMsg err;
  err.set_code(code);
  err.set_msg(msg);
  std::string body;
  err.SerializeToString(&body);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_Error), body));
}

void Dispatcher::handleLogin(net::WsSession& s, const std::string& body) {
  LoginReq req;
  if (!req.ParseFromString(body)) {
    sendError(s, 400, "parse LoginReq failed");
    return;
  }
  const int64_t playerId = nextPlayerId_++;
  s.setPlayerId(playerId);
  LoginAck ack;
  ack.set_code(0);
  ack.set_player_id(playerId);
  ack.set_msg("ok");
  std::string out;
  ack.SerializeToString(&out);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_LoginAck), out));
  GD_LOG_INFO("login uid={} -> playerId={}", req.uid(), playerId);
}

void Dispatcher::handleCreateRoom(net::WsSession& s, const std::string& body) {
  CreateRoomReq req;
  if (!req.ParseFromString(body)) {
    sendError(s, 400, "parse CreateRoomReq failed");
    return;
  }
  const std::string roomId = rooms_.createRoom(s.playerId());
  s.setRoomId(roomId);
  CreateRoomAck ack;
  ack.set_code(0);
  ack.set_room_id(roomId);
  ack.set_msg("ok");
  std::string out;
  ack.SerializeToString(&out);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_CreateRoomAck), out));
  GD_LOG_INFO("create room {} by playerId={}", roomId, s.playerId());
}

void Dispatcher::handleJoinRoom(net::WsSession& s, const std::string& body) {
  JoinRoomReq req;
  if (!req.ParseFromString(body)) {
    sendError(s, 400, "parse JoinRoomReq failed");
    return;
  }
  if (!rooms_.joinRoom(req.room_id(), s.playerId())) {
    sendError(s, 404, "room not found or full");
    return;
  }
  s.setRoomId(req.room_id());

  RoomState state;
  state.set_code(0);
  state.set_room_id(req.room_id());
  const room::Room* room = rooms_.find(req.room_id());
  if (room) {
    for (const auto& p : room->players) {
      auto* rp = state.add_players();
      rp->set_player_id(p.playerId);
      rp->set_name(p.name);
      rp->set_seat(p.seat);
    }
  }
  std::string out;
  state.SerializeToString(&out);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_RoomState), out));
  GD_LOG_INFO("join room {} playerId={}", req.room_id(), s.playerId());
}

void Dispatcher::handleHeartbeat(net::WsSession& s, const std::string& body) {
  HeartbeatReq req;
  if (!req.ParseFromString(body)) {
    sendError(s, 400, "parse HeartbeatReq failed");
    return;
  }
  s.markHeartbeat(util::nowMs());
  HeartbeatAck ack;
  ack.set_ts(req.ts());
  std::string out;
  ack.SerializeToString(&out);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_HeartbeatAck), out));
}

void Dispatcher::handleReconnect(net::WsSession& s, const std::string& body) {
  ReconnectReq req;
  if (!req.ParseFromString(body)) {
    sendError(s, 400, "parse ReconnectReq failed");
    return;
  }
  s.setPlayerId(req.player_id());
  s.setRoomId(req.room_id());

  RoomState state;
  state.set_code(0);
  state.set_room_id(req.room_id());
  const room::Room* room = rooms_.find(req.room_id());
  if (room) {
    for (const auto& p : room->players) {
      auto* rp = state.add_players();
      rp->set_player_id(p.playerId);
      rp->set_name(p.name);
      rp->set_seat(p.seat);
    }
  } else {
    state.set_code(404);
    state.set_msg("room not found");
  }
  std::string out;
  state.SerializeToString(&out);
  s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_RoomState), out));
  GD_LOG_INFO("reconnect playerId={} roomId={}", s.playerId(), s.roomId());
}

}  // namespace guandan::proto
