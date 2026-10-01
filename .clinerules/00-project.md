# 项目：掼蛋对战 APP（Guandan）

## 角色
你是本项目的架构主程。精通 Cocos Creator 3.8 + TypeScript 客户端，C++20 游戏服务器，以及棋牌类 AI 启发式策略设计。

## 项目目标
开发一款 4 人 2v2 掼蛋对战 APP：
- 真人在线对战
- 支持 AI Bot 补位
- 支持“提示出牌”和“教学讲解”
- 客户端 Cocos Creator 原生包（Android/iOS），后续可扩展 H5
- 通信统一用 WebSocket

## 技术栈
- 客户端：Cocos Creator 3.8.x + TypeScript
- 服务端：C++20 + CMake
- 通信：**WebSocket（ws:// 开发 / wss:// 生产）+ 二进制 Protobuf**
- WebSocket 服务端库：优先 uWebSockets（备选 websocketpp + Boost.Asio）
- 日志：spdlog
- 单测：googletest
- 存储：后期 SQLite，暂不接 MySQL/Redis

## 为什么用 WebSocket（不是裸TCP）
- Cocos 原生支持 WebSocket，不用自写 socket 解包
- 后续出 H5 / 微信小游戏可直接复用同一套协议
- 走 80/443 端口，移动网络 NAT 穿透更稳
- 帧边界由 WS 协议保证，我们只需在二进制帧内定业务包头
- 性能对棋牌完全够用（消息频率低，不卡）

## 架构铁律
1. 服务器权威：出牌合法性/比牌/升级/接风全在服务端
2. 客户端只渲染+输入+发请求+收推送
3. 规则引擎纯函数，不依赖网络/IO
4. AI 决策在服务端，客户端不计算
5. WebSocket 二进制帧内格式：[2B cmd][2B ver][4B len][protobuf body]（小端）
6. 每个 WS 帧 = 一条完整业务消息（不跨帧拼包）

## 开发顺序（按此执行，不可跳）
1. Phase 01：WebSocket 通信能力（连上+握手+登录+建房+心跳回环）
2. Phase 02：掼蛋核心逻辑（纯 C++ 规则引擎 + 单测）
3. Phase 03：联机对战跑通（服务端驱动一局，客户端跟帧）
4. Phase 04：AI Bot + 提示出牌
5. Phase 05：教学能力（出牌理由标签 + 教学气泡）
6. Phase 06：前端场景布置（美术/动画/音效）
7. Phase 07+：匹配/段位/打包

> 前端美术最后做，前期用占位 UI。