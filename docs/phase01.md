# Phase 01：WebSocket 通信能力

完成「连上 + 握手 + 登录 + 建房 + 心跳回环」，不含任何出牌逻辑。

## 文件清单

### 共享协议
- `guandan-server/src/proto/messages.proto` — Phase 01 全部消息定义（登录/建房/入房/房间快照/重连/心跳/错误）

### 服务端（C++20，guandan-server/）
- `src/net/Frame.h/.cpp` — 业务帧编解码 `[2B cmd][2B ver][4B len][body]` 小端
- `src/net/WsSession.h/.cpp` — 单连接：RFC6455 握手 + 帧收发（解掩码/ping-pong/close）
- `src/net/WsServer.h/.cpp` — 单线程 poll 事件循环 + 心跳超时主动 close
- `src/util/Crypto.h/.cpp` — SHA1 / Base64 / Sec-WebSocket-Accept（纯函数）
- `src/util/Log.h` — spdlog 封装（缺失时退回内置极简日志）
- `src/util/Random.h` — xorshift32 随机（房间号）
- `src/util/Time.h` — 单调时钟 nowMs()
- `src/proto/Cmd.h` — 命令号常量
- `src/proto/Dispatcher.h/.cpp` — 业务帧分发 + 登录/建房/入房/心跳/重连处理器
- `src/room/RoomManager.h/.cpp` — 最小房间注册（建房/入房/快照，满员 4 人）
- `src/main.cpp` — 入口，监听 9001
- `CMakeLists.txt` — 构建（anaconda protobuf 探测 + protoc 生成 + core/exe/test）

### 服务端测试（gtest 替代：极简 minitest）
- `tests/minitest.h` — 极简测试框架（TEST/EXPECT_*，可无缝切换 gtest）
- `tests/test_frame.cpp` — 帧编解码/小端/短帧
- `tests/test_crypto.cpp` — SHA1/Base64/WS-Accept 已知向量
- `tests/test_proto.cpp` — protobuf 往返 + RoomManager
- `tests/test_ws_integration.cpp` — 端到端：握手 + 登录 + 心跳回环

### 客户端（Cocos Creator 3.8，guandan-client/）
- `assets/scripts/config/protoId.ts` — cmd 常量（与 Cmd.h 一致）
- `assets/scripts/net/frame.ts` — 业务帧编解码（DataView 小端）
- `assets/scripts/net/pb/wire.ts` — protobuf wire 读写 + 手写 UTF-8
- `assets/scripts/net/pb/messages.ts` — 消息类型 + 编解码（与 .proto 字段一致）
- `assets/scripts/net/pb/index.ts` — re-export
- `assets/scripts/net/WsClient.ts` — WebSocket 封装（连接/发送/onMessage/心跳 5s/指数退避重连 1s→8s）
- `tsconfig.json` — 独立类型检查配置

## 验证命令

```bash
# 服务端构建 + 测试
cd guandan-server
cmake -S . -B build
cmake --build build -j4
cd build && ctest --output-on-failure   # 4/4 通过

# 启动服务端
./guandan_server 9001

# 客户端类型检查
cd guandan-client
tsc --noEmit
```

## 已知限制（重要）

1. **WebSocket 服务端为手写最小实现**（RFC6455 握手 + 帧收发，单线程 poll）。
   - 原因：`uWebSockets`/`websocketpp` 均不在 Homebrew，且 GitHub 源被墙无法 clone。
   - 已按 `WsServer/WsSession` 抽象，联网后可无痛替换为 uWebSockets。
2. **spdlog 未安装**（brew 3.6.16 在 macOS 15 上不可用，`brew install` 报
   `unknown or unsupported macOS version: :dunno`）。`util/Log.h` 提供 `{}` 占位符的
   极简回退，联网后 `brew install spdlog` 即自动启用真 spdlog。
3. **googletest 未安装**（同上原因）。用 `tests/minitest.h` 极简框架替代，宏语义对齐
   googletest，后续可无缝切换。
4. **protobuf 依赖 anaconda**（`/opt/anaconda3` 的 protoc 3.20.3 + 动态库），运行时依赖
   `libprotobuf.dylib`，CMake 已配置 rpath。无 anaconda 环境需自行装 protobuf。
5. **客户端 TS 编解码为手写**（非 protobufjs 生成）。原因：npm 可达但为保持零依赖/二进制
   兼容已用 `protoc --encode` 双向验证字节一致（LoginReq/HeartbeatReq/CreateRoomReq/LoginAck/RoomState）。
   后续可换 pbjs 生成。
6. 客户端未接 Cocos 引擎（本环境无 Cocos Creator），仅通过 `tsc --noEmit` 验证类型；
   `WebSocket` 使用全局浏览器式 API（Cocos 3.8 浏览器/原生 jsb 通用）。
7. int64 在 TS 端用 `number` 表示（受 JS 安全整数 2^53 限制），Phase 01 的 playerId/ts
   均远小于该值，无精度问题；后续若需更大 id 再改 BigInt。
