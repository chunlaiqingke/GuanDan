// 命令号常量，与服务端 src/proto/Cmd.h 保持一致。
export const Cmd = {
  C2S_Login: 1001,
  S2C_LoginAck: 1002,

  C2S_CreateRoom: 2001,
  S2C_CreateRoomAck: 2002,
  C2S_JoinRoom: 2003,
  S2C_RoomState: 2004,
  C2S_Reconnect: 2005,

  S2C_TurnStart: 3001,
  S2C_Tick: 3002,
  S2C_AutoPlay: 3003,
  S2C_HostMode: 3004,
  C2S_SetHost: 3005,
  C2S_Play: 3006,
  C2S_Pass: 3007,
  S2C_Deal: 3008,
  S2C_RoundEnd: 3009,
  S2C_GameStart: 3010,
  S2C_PlayResult: 3011,
  S2C_PassResult: 3012,
  C2S_AddBot: 3013,
  C2S_Hint: 3014,
  S2C_HintAck: 3015,
  C2S_StartMatch: 3016,
  C2S_CancelMatch: 3017,
  S2C_MatchAck: 3018,
  S2C_RankUpdate: 3019,

  C2S_Heartbeat: 9001,
  S2C_HeartbeatAck: 9002,
  S2C_Error: 9999,
} as const;

export type CmdValue = (typeof Cmd)[keyof typeof Cmd];
