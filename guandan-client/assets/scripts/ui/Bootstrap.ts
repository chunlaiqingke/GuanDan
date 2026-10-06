import { _decorator, Component } from 'cc';
import { bootstrapClient, ClientSession } from './Scenes';

const { ccclass } = _decorator;

// 占位启动组件：挂到场景任意节点即可连接服务端，先用文字视图(ConsoleView)驱动。
// 后续在 Cocos 里实现 TableView 接口的组件，替换 Scenes 里的 ConsoleView 即可接上真实渲染。
@ccclass('Bootstrap')
export class Bootstrap extends Component {
  session: ClientSession | null = null;

  start(): void {
    // 开发环境连本机服务端；生产改 wss://域名/ws
    this.session = bootstrapClient('ws://127.0.0.1:9001');
  }
}
