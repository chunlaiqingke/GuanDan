# Cocos Creator 客户端约束

- Cocos Creator 3.8.x + TypeScript
- 目录：
  - assets/scripts/net/WsClient.ts   # WebSocket 封装（用引擎内置 WebSocket）
  - assets/scripts/net/pb/           # protobuf ts 生成代码
  - assets/scripts/game/             # 牌桌状态机（只读服务端数据）
  - assets/scripts/ui/               # 大厅/房间/牌桌/结算
  - assets/scripts/ai/               # 仅提示展示
  - assets/scripts/config/protoId.ts # cmd 常量

## 网络使用规范
- 禁止用 XMLHttpRequest / fetch 做对战消息
- 禁止用 JSON 对战协议
- 只用 WebSocket 二进制帧
- 示例：this.ws.send(encode(CMD.C2S_Login, LoginReq.encode({uid, token}).finish()))
- Phase 01~03 用占位 UI（文字牌面+按钮），不接美术
- 所有出牌必须等 S2C 回包再刷新，不本地预判
- 不用 any，全显式类型