# C++ 服务端约束

- C++20，CMake 3.20+
- WebSocket 服务用 **uWebSockets**（推荐）或 websocketpp
- 目录：
  - src/net/          # WsServer / WsSession（封装 uWS）
  - src/proto/        # .proto + 生成代码 + 分发器
  - src/guandan/      # 规则引擎纯函数
  - src/guandan/ai/   # 启发式决策
  - src/room/         # Room / GameTable 状态机
  - src/util/         # spdlog / random / time

## WebSocket 服务端要求
- uWebSockets App 监听 9001（ws）
- 每个连接 = 一个 WsSession（存 playerId / roomId）
- 收到二进制消息 → 解 [cmd][ver][len][body] → protobuf 解析 → 派发
- 发送：把 protobuf 序列化后前面拼 cmd+ver+len，ws.send(binary)
- 不处理分片帧（我们每条消息单帧发完）
- 生产环境由 nginx 做 wss 终止，后端仍 ws

## Phase 01 只做
- uWS 起服务
- onMessage 解包打印
- 登录回包 / 建房回包 / 心跳回包
- 不出牌逻辑

## 禁止
- Phase 01 不写牌型识别
- 不引入 Redis/MySQL
- 不用 std::cout，统一 spdlog
- 牌值用整数编码，不存字符串