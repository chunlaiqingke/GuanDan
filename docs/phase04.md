# Phase 04：AI Bot + 提示出牌

完成服务端启发式决策层 + AI Bot 补位 + 提示出牌（Top3），全部在服务端，客户端不计算 AI 决策。

## 文件清单

### 服务端（guandan-server/src/）
- `guandan/ai/PlayGen.h/.cpp`（新增）— 合法出牌枚举 `generatePlays`（含逢人配配牌）+ 最小压牌 `minBeat`。
- `guandan/ai/Strategy.h/.cpp`（新增）— 决策 `decide`（难度档 Easy/Normal/Hard）+ 提示 `hint`（Top3）+ `ReasonTag` 理由标签。
- `room/RoomManager.h/.cpp`（扩展）— `RoomPlayer.isBot` + `addBot`。
- `proto/messages.proto`（扩展）— `AddBotReq`/`HintReq`/`HintAck`/`HintPlay`；`RoomPlayer.is_bot`。
- `proto/Cmd.h`（扩展）— 3013~3015。
- `proto/Dispatcher.h/.cpp`（扩展）— `handleAddBot`/`handleHint`/`botResolve`；`onTick` 改为一次 tick 内连续清掉 Bot/托管/超时回合（避免每个 bot 回合等 200ms）。

### 服务端测试（guandan-server/tests/）
- `test_ai.cpp`（新增）— 9 用例：枚举（单/对/顺/逢人配）、minBeat、决策（走完/领出/保炸弹/最小压）、提示 Top3。
- `test_game_flow.cpp`（扩展）— `GameFlowWithBots`：1 人类 + 3 Bot 自动跑完一盘。

### 客户端（guandan-client/assets/scripts/）
- `config/protoId.ts`（扩展）— 3013~3015。
- `net/pb/messages.ts`（扩展）— `AddBotReq`/`HintAck` 编解码；`RoomPlayer.isBot`。
- `game/GameState.ts`（扩展）— `hint`/`botUids` + `applyHintAck`/`applyRoomState`/`isBot`。

## 验证命令

```bash
# 服务端
cd guandan-server
cmake -S . -B build
cmake --build build -j4
cd build && ctest --output-on-failure   # 8/8 通过

# 客户端类型检查
cd guandan-client
tsc --noEmit                             # 0 错误
```

## 已实现策略（Normal 难度启发式）

1. 能走完优先（`GoOut`；跟牌时校验能压过）。
2. 领出：出有效点最小的单张（保住级牌/王）。
3. 跟牌：最小压牌（`MinBeat`）；需用炸弹时对手牌多则让（`SaveBomb`）、对手快走完则堵（`BlockOpp`）；队友领先让牌（`LetTeammate`）。
4. 提示 `hint`：排序（能走完 → 非炸弹 → 有效点 → 张数）返回 Top3。

## 已知限制 / 假设

1. **难度档**：Easy（可过就过）/ Normal（启发式）已实现；Hard 暂按 Normal 处理（记牌器 + 队友配合权重留后续）。
2. **领出为 v1 启发式**（最小有效点单张），未做"最小手数"的完整拆牌分解。
3. **逢人配生成**：用"自然牌 + 逢人配补缺"（≤2 张逢人配）；牌型最终以 `classify` 为准，保证与规则引擎一致。
4. **兜底**：AI 决策与规则引擎罕见不一致时，`botResolve` 兜底（过牌/最小单张），保证回合不卡死。
5. Bot 用负 playerId 区分，客户端通过 `is_bot` 字段识别（负 int64 无法在 JS 直接还原，故显式传 is_bot）。
6. `ReasonTag` 已定义（GoOut/MinBeat/SaveBomb/LetTeammate/BlockOpp），Phase 05 教学直接复用。

## 下一步

Phase 05：教学能力（出牌理由标签 + 教学气泡），复用 ReasonTag + 客户端白话讲解。
