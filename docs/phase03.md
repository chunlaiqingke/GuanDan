# Phase 03：联机对战跑通（服务端驱动一局，客户端跟帧）

完成「发牌 → 出牌循环 → 头/二/三/末游 → 升级」的单盘对战，服务端权威；客户端只跟随 S2C 帧渲染。含 15s 倒计时、超时自动出牌、托管、断线托管、重连续时。

## 文件清单

### 服务端（guandan-server/src/）
- `room/GameTable.h/.cpp`（新增）— 单盘状态机：发牌 / 回合循环 / 出牌·过牌校验 / 名次 / 升级 / 接风 / 进贡纯计算。纯逻辑，不依赖网络。
- `room/RoomManager.h/.cpp`（扩展）— `Room` 挂 `GameTable` + 对战状态（level/deadline/hostMode/timeoutCount）；新增 `findMutable` / `forEachRoom` / `seatOf`。
- `net/WsServer.h/.cpp`（扩展）— 新增 `setOnTick`，事件循环每轮回调（驱动倒计时/超时/托管）。
- `proto/messages.proto`（扩展）— Phase 03 消息（TurnStart/Tick/AutoPlay/HostMode/SetHostReq/PlayReq/PassReq/Deal/RoundEnd/GameStart/PlayResultMsg/PassResultMsg）。
- `proto/Cmd.h`（扩展）— 3001~3012 命令号。
- `proto/Dispatcher.h/.cpp`（重写）— 对战编排：开局/出牌/过牌/托管/倒计时/超时自动/断线托管/重连补发；`playerId→session` 映射用于广播。
- `main.cpp` — 接入 `setOnTick`。

### 服务端测试（guandan-server/tests/）
- `test_gametable.cpp`（新增）— 8 用例：发牌 / 回合顺序与校验 / 领出不能过 / 非法牌 / 名次+升级+2 / 升级+1 / 进贡纯计算 / 最小单张。
- `test_game_flow.cpp`（新增）— 端到端：4 个 WS 客户端登录→建房→入房→自动开局→按「领出最小单张/跟牌过」跑完一盘→收到 RoundEnd。

### 客户端（guandan-client/assets/scripts/）
- `config/protoId.ts`（扩展）— 3001~3012 常量。
- `net/pb/messages.ts`（扩展）— Phase 03 消息类型 + 编解码（含 packed repeated varint 读取）。
- `game/GameState.ts`（新增）— 牌桌状态模型（只读服务端数据）：手牌/座位/当前回合/倒计时/桌面最大一手/名次，订阅 S2C 帧更新。

## 验证命令

```bash
# 服务端
cd guandan-server
cmake -S . -B build
cmake --build build -j4
cd build && ctest --output-on-failure   # 7/7 通过

# 客户端类型检查
cd guandan-client
tsc --noEmit                             # 0 错误
```

## 已实现规则

1. **单盘流程**：4 人满自动开局（级牌取房主 `CreateRoomReq.rule.level`，缺省 2）→ 发 4×27 → 首出 seat0 → 跟牌/过 → 一圈 3 过判定赢家继续出。
2. **出牌校验**（服务端权威）：牌型非法 / 手牌不含 / 不能压过 / 领出不能过，分别回错误码。
3. **名次**：出完牌按顺序记 头游/二游/三游，第 4 人末游；3 人出完即 RoundEnd。
4. **升级**：头游+二游同队 +2，否则 +1（打 A 过关属多局，未接）。
5. **接风**：出完最后牌且无人压 → 队友接风（已接进循环）。
6. **倒计时**：普通回合 15s；最后 5s 每秒推 `S2C_Tick`；归零自动出（跟牌可过→自动过，首出→最小单张）。
7. **托管**：`C2S_SetHost` 切换；断线自动进入托管态；托管态服务端代出（最简策略）。

## 已知限制 / 假设

1. **进贡未接循环**：`computeTribute` 已实现为纯函数 + 单测，但单盘范围不触发；连续多局 + 进贡交换留到后续。
2. **多局打级未做**：`levelUp` 只算 +1/+2，「打 A 需头游且非末游过关」的多局逻辑未接。
3. **托管代出用最简策略**（跟牌过/领出最小单张），Phase 04 换启发式 AI。
4. **发牌种子**用 `nowMs`（生产可换 `std::random_device`）。
5. **倒计时粒度**：poll 200ms；`S2C_Tick` 在最后 5s 每秒推一次（够用，后续可精化）。
6. **客户端无 Cocos 引擎**，仅 `tsc --noEmit` 验证类型；`GameState` 只读服务端数据，不本地判牌/判超时。
7. 房主入座 seat0、加入顺序即 seat 顺序；`RoomPlayer.name` 仍未填充（前端占位阶段）。

## 下一步

Phase 04：AI Bot + 提示出牌（规则层已就绪，接决策层：拆牌评估 + Top3 推荐 + 托管/超时复用启发式策略）。
