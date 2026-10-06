# Phase 07：匹配 / 段位 / 打包

完成匹配队列（满 4 人开局 / 超时补 Bot）、段位 ELO + SQLite 持久化、服务端 CPack 打包 + Dockerfile 骨架。

## 文件清单

### 服务端（guandan-server/src/）
- `rank/Rating.h/.cpp`（新增）— 2v2 团队 ELO `computeRatings` + 段位 `tierName`。
- `rank/RatingStore.h/.cpp`（新增）— SQLite 持久化 playerId→rating（`:memory:` 可测）。
- `proto/messages.proto`（扩展）— `StartMatchReq`/`CancelMatchReq`/`MatchAck`/`RankInfo`/`RankUpdate`。
- `proto/Cmd.h`（扩展）— 3016~3019。
- `proto/Dispatcher.h/.cpp`（扩展）— 匹配队列 + `matchTick`（满 4 人/超时补 Bot）、RoundEnd 段位结算广播、`openRatings`。
- `main.cpp` — 启动时 `openRatings(argv[2] 或 guandan.db)`。

### 构建/打包
- `CMakeLists.txt`（扩展）— sqlite3 探测/链接、`src/rank/*.cpp`、`install` + CPack（TGZ）。
- `Dockerfile`（新增）— 多阶段构建骨架。

### 服务端测试（guandan-server/tests/）
- `test_rank.cpp`（新增）— ELO 胜负/爆冷、段位档位、RatingStore 往返。
- `test_game_flow.cpp`（扩展）— `GameFlowMatchmaking`（4 人匹配→MatchAck+Deal）、`GameFlowWithBots` 追加段位结算断言。

### 客户端（guandan-client/assets/scripts/）
- `config/protoId.ts`（扩展）— 3016~3019。
- `net/pb/messages.ts`（扩展）— `MatchAck`/`RankInfo`/`RankUpdate` 编解码。
- `game/GameState.ts`（扩展）— `rankUpdates`/`matchAck` + apply 方法。
- `ui/LobbyController.ts`（扩展）— `match()`/`cancelMatch()`。
- `ui/TableView.ts` / `ConsoleView.ts`（扩展）— `onMatch`/`onRankUpdate`。
- `ui/TableController.ts`（扩展）— 绑定 `S2C_MatchAck`/`S2C_RankUpdate`。

## 验证命令

```bash
# 服务端
cd guandan-server
cmake -S . -B build
cmake --build build -j4
cd build && ctest --output-on-failure   # 9/9 通过
cpack -G TGZ                            # 产出 guandan-server-0.1.0-*.tar.gz

# 客户端类型检查
cd guandan-client
tsc --noEmit                            # 0 错误
```

## 段位规则

- ELO：团队平均分、K=32，胜队 `+K*(1-E)`、负队 `-K*(1-E)`；初始 1200。
- 档位：青铜(<1200) / 白银(1200) / 黄金(1400) / 铂金(1600) / 钻石(1800) / 王者(2000)。

## 已知限制 / 假设

1. **匹配策略最简**：满 4 人立即匹配、超时 30s 用现有 Bot 补位；未做段位/MMR 加权匹配。
2. **段位 ELO**：团队平均分 + K=32，档位阈值固定；多局打 A 过关等未接入。
3. **SQLite**：字符串拼 SQL（playerId/rating 为 int64/int，无注入风险），未用 prepared statement。
4. **客户端 delta**：由 `new-old` 计算（负 int32 的 protobuf 10 字节变长编码在 JS 端有精度问题，故跳过 delta 字段）。
5. **Dockerfile 为骨架**：本环境无 Docker 未实测；CPack TGZ 已验证通过。
6. **客户端 Cocos 打包**：需 Cocos Creator（本环境无），只写构建说明。

## 客户端 Cocos 打包说明（后续在 Cocos Creator 中执行）

1. 用 Cocos Creator 3.8 打开 `guandan-client/` 项目。
2. 菜单「项目 → 构建发布」；平台选 Android / iOS / Web Mobile。
3. 配置服务器地址：开发 `ws://ip:9001`、生产 `wss://domain/ws`。
4. 原生包需安装对应平台 SDK（Android SDK/NDK、Xcode）。
5. `assets/scripts/` 已引擎无关（渲染走 `TableView` 接口），Cocos 里实现 `TableView` 组件即可接上。
