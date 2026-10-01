#pragma once

#include <cstdint>

namespace guandan::proto {

  // 命令号（与客户端 assets/scripts/config/protoId.ts 保持一致）
  enum class Cmd : uint16_t {
    C2S_Login = 1001,
    S2C_LoginAck = 1002,

    C2S_CreateRoom = 2001,
    S2C_CreateRoomAck = 2002,
    C2S_JoinRoom = 2003,
    S2C_RoomState = 2004,
    C2S_Reconnect = 2005,

    C2S_Heartbeat = 9001,
    S2C_HeartbeatAck = 9002,
    S2C_Error = 9999,
  };

}  // namespace guandan::proto
