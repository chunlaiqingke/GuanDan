# 对战流程控制（Battle Flow）

## 作用范围
本文件管“一局对战里的时序与回合状态”，不管牌型大小（那归 02-guandan-rules）。
Phase 03 起生效，Phase 01/02 不涉及。

## 回合状态机（服务端权威）
- Idle → Dealing → WaitPlay(当前出牌人) → Playing
- WaitPlay(下家) / Pass
- RoundEnd → Upgrade → NextRound
- 每个 WaitPlay 状态带倒计时
- 状态变更必须广播 S2C_RoomState（含当前操作人 + remainMs）

## 出牌倒计时规则
- 普通回合：15 秒
- 最后剩 5 秒：服务端推 S2C_Tick{remainMs}（每1s一次，客户端显示红字）
- 倒计时归零未操作 → 服务端自动判定：
  - 若本回合是“跟牌且可过” → 自动 Pass
  - 若本回合是“首出” → 自动出最小单张（或最小合法组）
  - 记录超时次数，超 3 次标记“可能挂机”
- 炸弹/关键回合不额外加时（初版先统一 15s，后续可配）

## 托管（AFK / 断线）
- 玩家断线 → 不踢出房间，进入“托管态”
- 托管态下服务端用 AI Bot 策略代出（复用 Phase 04 的普通难度决策）
- 客户端显示“对方托管中”
- 重连成功 → 推房间快照 + 当前 remainMs，继续原倒计时（不重置）
- 主动点“托管”按钮也进入托管态

## 重连后倒计时恢复
- S2C_RoomState 带 `currentTurnUid` + `deadlineTs(ms)`
- 客户端用 deadlineTs - 本地时间 算剩余，避免时钟漂移
- 不允许客户端自己倒计时当权威，只做显示

## 消息补充（在 01-comm-protocol 基础上追加）
| Cmd | 方向 | 说明 |
|---|---|---|
| 3001 S2C_TurnStart | S→C | uid + deadlineTs + 可操作类型(play/pass) |
| 3002 S2C_Tick | S→C | remainMs（最后5s用） |
| 3003 S2C_AutoPlay | S→C | uid + 自动出的牌（超时/托管触发） |
| 3004 S2C_HostMode | S→C | uid + bool isHost(是否托管) |
| 3005 C2S_SetHost | C→S | 主动申请/取消托管 |

## 服务端实现约束
- 倒计时用单线程事件循环里的定时器（uWS + 定时器，或 Asio steady_timer）
- 不允许每个连接开独立 sleep 线程
- deadline 存 Room 对象里，不存 Session
- 房间所有定时器随 Room 销毁自动清

## 客户端表现约束
- 倒计时环/数字放牌桌对应座位
- 最后5秒变红 + 轻微震动（移动端）
- 不本地判定超时，只按 S2C_Tick 显示
- 托管态按钮置灰，显示“托管中”

## 禁止
- 禁止客户端自己算“到时间没出牌就自动出”（必须服务端推 AutoPlay）
- 禁止倒计时归零后还允许客户端发 C2S_Play
- 禁止重连后重置整轮倒计时（只续算剩余）