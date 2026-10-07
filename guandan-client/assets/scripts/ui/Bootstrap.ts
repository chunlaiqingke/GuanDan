import { _decorator, Component } from 'cc';
import { WsClient } from '../net/WsClient';
import { LobbyController } from './LobbyController';
import { TableController } from './TableController';
import { ConsoleView } from './ConsoleView';

const { ccclass } = _decorator;

// 占位启动组件：挂到场景任意节点后点「预览」即可自动运行。
// 连上服务端后：登录 → 建房 → 补 3 个 Bot（满 4 人自动开局），用文字视图(ConsoleView)在控制台打印整局。
@ccclass('Bootstrap')
export class Bootstrap extends Component {
  start(): void {
    const ws = new WsClient();
    const lobby = new LobbyController(ws);
    new TableController(ws, new ConsoleView()); // 绑定 S2C → 文字视图

    ws.connect('ws://127.0.0.1:9001', () => {
      lobby.login('demo_' + Date.now(), 'token');
      lobby.createRoom(2); // 级牌打 2
      lobby.addBot(3);     // 补 3 个 Bot → 满 4 人自动开局
    });
  }
}
