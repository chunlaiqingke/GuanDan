# Phase 06：前端场景布置（美术/动画/音效）

交付类型安全的前端场景/UI 层（引擎无关）+ 文字牌面占位。本环境无 Cocos Creator，无法产出 `.scene`/贴图/动画/音效资产，故以「渲染接口 + 占位文字视图」落地，后续在 Cocos Creator 里接上真实美术即可。

## 文件清单（guandan-client/assets/scripts/）

### ui/（大厅/房间/牌桌/结算）
- `cardDisplay.ts`（新增）— 文字牌面：`cardLabel`（"♠A"/"♥10"/"大王"）+ `sortHand`。
- `TableView.ts`（新增）— 牌桌渲染接口（引擎无关），Cocos 组件实现它即可接上渲染。
- `TableController.ts`（新增）— 跟帧控制器：绑定 WsClient 的 S2C 帧 → 更新 `GameState` → 驱动 `TableView`；暴露 play/pass/hint/setHost/addBot 动作。
- `LobbyController.ts`（新增）— 大厅/房间：登录、建房、入房、补位机器人。
- `ConsoleView.ts`（新增）— 占位文字视图（实现 `TableView`，控制台文本渲染）。
- `Scenes.ts`（新增）— 占位场景引导 `bootstrapClient`（连接 → 大厅 → 牌桌）。

### ai/（仅提示展示）
- `HintDisplay.ts`（新增）— 把 Top3 推荐（含理由标签）转为「高亮牌 + 白话讲解」。

### fx/（音效/动画占位）
- `Sfx.ts`（新增）— `SfxHooks` 接口 + `noopSfx` 空实现（Cocos 里替换为真实音效/动画）。

## 验证命令

```bash
cd guandan-client
tsc --noEmit   # 0 错误
```

## 已实现的客户端目录结构（对照 04-client-cocos）

```
assets/scripts/
├─ config/protoId.ts     # cmd 常量
├─ net/                  # WsClient + frame + pb 编解码
├─ game/                 # GameState（牌桌状态机）+ teaching（教学文案）
├─ ui/                   # 大厅/房间/牌桌/结算（本次新增）
├─ ai/                   # 提示展示（本次新增）
└─ fx/                   # 音效/动画桩（本次新增）
```

## 已知限制 / 假设

1. **本环境无 Cocos Creator**：无法产出 `.scene`/`.prefab`/贴图/动画/音效资产；仅交付 TS 场景/UI 逻辑，用 `tsc --noEmit` 验证类型。
2. **渲染通过 `TableView` 接口抽象**：Cocos 组件实现该接口（替换 `ConsoleView`）即可接上真实渲染。
3. **音效/动画用 `noopSfx` 桩**：Cocos 里替换为真实实现。
4. `bootstrapClient` 仅演示接线（`new WsClient()` 依赖浏览器/Cocos 的 `WebSocket`），需在浏览器或 Cocos 环境运行；本环境无 WebSocket 运行时，故未做运行态联调。
5. 美术/动画/音效的具体表现（牌桌布局、倒计时环、气泡动画）需在 Cocos Creator 编辑器里布置。

## 下一步

Phase 07+：匹配 / 段位 / 打包（后续规划）。
