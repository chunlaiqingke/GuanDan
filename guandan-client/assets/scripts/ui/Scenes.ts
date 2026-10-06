// 占位场景引导：连接 → 大厅 → 牌桌（ConsoleView 文字渲染）。
// Cocos 接入时：把 ConsoleView 换成挂在 Canvas 上的 Cocos 组件（实现 TableView）。

import { WsClient } from '../net/WsClient';
import { ConsoleView } from './ConsoleView';
import { LobbyController } from './LobbyController';
import { TableController } from './TableController';

export interface ClientSession {
  ws: WsClient;
  lobby: LobbyController;
  table: TableController;
}

export function bootstrapClient(url: string): ClientSession {
  const ws = new WsClient();
  const view = new ConsoleView();
  const lobby = new LobbyController(ws);
  const table = new TableController(ws, view);
  ws.connect(url);
  return { ws, lobby, table };
}
