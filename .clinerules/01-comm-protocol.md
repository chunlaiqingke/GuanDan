# WebSocket 通信规范（Phase 01 生效）

## 传输方式
- 开发环境：ws://ip:9001
- 生产环境：wss://domain/ws （走 TLS，由 nginx/caddy 终止）
- 数据格式：WebSocket 二进制帧（opcode=0x2）
- 不使用文本帧，不用 JSON 做对战协议

## 帧内业务包格式（每帧一条完整消息）
- 小端字节序
- version 当前固定 1
- bodyLen = protobuf 序列化后字节数
- 一条 WS 帧只装一条业务消息，不粘不拆

## 心跳
- 客户端每 5s 发 C2S_Heartbeat（WS 二进制帧）
- 服务端收到立即回 S2C_HeartbeatAck
- 连续 3 次没收到心跳 → 服务端主动 close

## 基础消息（Phase 01 必做）
| Cmd | 方向 | 说明 |
|---|---|---|
| 1001 C2S_Login | C→S | uid/token |
| 1002 S2C_LoginAck | S→C | code/playerId |
| 2001 C2S_CreateRoom | C→S | 规则配置 |
| 2002 S2C_CreateRoomAck | S→C | roomId |
| 2003 C2S_JoinRoom | C→S | roomId |
| 2004 S2C_RoomState | S→C | 房间快照 |
| 9001 C2S_Heartbeat | C→S | ts |
| 9002 S2C_HeartbeatAck | S→C | ts |
| 9999 S2C_Error | S→C | code+msg |

## 服务端库选择
- 首选 **uWebSockets**（头文件库，基于 libuv/事件循环，性能极高，C++20 友好）
- 备选 **websocketpp**（基于 Boost.Asio，成熟但重）
- Phase 01 先单线程事件循环跑通

## 客户端封装
- 用 Cocos 内置 `WebSocket`（cc.net.WebSocket 或 jsb 层）
- 封装在 `assets/scripts/net/WsClient.ts`
- 提供：connect / send(cmd, protoObj) / onMessage(cmd, cb) / reconnect()
- 自动把 protobuf 对象序列化成 [cmd][ver][len][body] 再 send

## 重连规则
- 断网自动重连，指数退避（1s,2s,4s,最大8s）
- 重连成功后发 C2S_Reconnect + roomId，服务端回房间快照