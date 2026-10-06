# Phase 05：教学能力（出牌理由标签 + 教学气泡）

完成服务端在 AI 出牌 / 提示出牌时附带 `reason_tag`，客户端把标签映射为白话讲解（教学气泡文案）。

## 文件清单

### 服务端（guandan-server/src/）
- `proto/messages.proto`（扩展）— `AutoPlay.reason_tag`（3003）、`HintPlay.reason_tag`（3015）。
- `guandan/ai/Strategy.h/.cpp`（扩展）— 新增 `HintEntry` + `hintTagged`（Top3 带理由标签）；`hint` 复用 `hintTagged`。
- `proto/Dispatcher.h/.cpp`（扩展）— `broadcastAction` 增加 `reasonTag`；`botResolve` 把 AI 决策标签带进 `S2C_AutoPlay`；`handleHint` 用 `hintTagged` 带出每条推荐理由。

### 服务端测试（guandan-server/tests/）
- `test_ai.cpp`（扩展）— `HintTaggedTags`：跟牌最小压打 MinBeat、领出打 None。

### 客户端（guandan-client/assets/scripts/）
- `net/pb/messages.ts`（扩展）— `AutoPlay.reasonTag`；`HintPlay`/`HintAck` 类型与解码。
- `game/teaching.ts`（新增）— `teachingText(tag)` 把标签映射为白话讲解。
- `game/GameState.ts`（扩展）— `hint: HintPlay[]`、`lastReasonTag`；`applyHintAck`/`applyAutoPlay` 记录标签。

## 理由标签（reason_tag，int32）

| 值 | 标签 | 白话讲解 |
|---|---|---|
| 0 | none | （无） |
| 1 | go_out | 这手牌能一次出完，直接走！ |
| 2 | min_beat | 用最小的牌压住，保留大牌 |
| 3 | beat_last | 压住上一手 |
| 4 | save_bomb | 先过一手，保留炸弹到关键回合 |
| 5 | let_teammate | 让牌给队友，队友领先 |
| 6 | block_opp | 对手快出完了，用炸弹堵住 |

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

## 已知限制 / 假设

1. **教学气泡 UI 归 Phase 06**：本阶段只做"标签产生 + 白话文案映射"，气泡渲染/动画/音效在 Phase 06 前端布置。
2. **LLM 自然语言教学未接入**：spec 里"客户端把 tag+上下文发给 LLM 生成自然语言教学"留到 Phase 05 后期/后续，当前用固定白话文案（零外调）。
3. **提示的 reason_tag 为启发式打标**：跟牌 Top 打 `min_beat`/`block_opp`，能走完打 `go_out`，领出无上一手时打 `none`。
4. 标签用 int32 传输，客户端 `teaching.ts` 与 `ReasonTag` 枚举保持映射一致。

## 下一步

Phase 06：前端场景布置（美术/动画/音效），用占位 UI 把牌桌、手牌、教学气泡、倒计时环等渲染出来。
