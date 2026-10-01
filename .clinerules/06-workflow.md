# 工作流约束

- 每次只做一个 phase，做完列：文件清单 / 验证命令 / 已知限制
- 用户说“开始 phase 0X”才进入
- 不跨 phase 预写代码
- C++ 改动带 gtest，TS 改动能过 Cocos 编译
- 通信层统一 WebSocket 二进制帧，不混用 TCP 裸流
- 规则歧义先问用户