// 全局会话（引擎无关）：跨场景共享的网络连接 + 控制器 + 流程事件。
// 场景组件（LoginScene/LobbyScene/TableScene）通过它执行动作，并监听流程事件切换场景。

import { WsClient } from '../net/WsClient';
import { LobbyController } from '../ui/LobbyController';
import { TableController } from '../ui/TableController';
import { ConsoleView } from '../ui/ConsoleView';
import { GameState } from './GameState';
import { Cmd } from '../config/protoId';
import { SERVER_URL } from '../config/network';
import {
  decodeCreateRoomAck,
  decodeError,
  decodeLoginAck,
  decodeMatchAck,
  decodeRoomState,
} from '../net/pb/messages';

export type FlowEvent = 'login' | 'roomEntered' | 'logout' | 'error';

export class Session {
  readonly ws: WsClient;
  readonly lobby: LobbyController;
  readonly table: TableController;

  roomId = '';
  playerId = 0;

  private listeners = new Map<FlowEvent, Array<{ cb: (arg?: unknown) => void; target?: unknown }>>();

  constructor(private readonly url: string) {
    this.ws = new WsClient();
    this.lobby = new LobbyController(this.ws);
    this.table = new TableController(this.ws, new ConsoleView());
    this.ws.connect(url);
    this.bindFlow();
  }

  get state(): GameState {
    return this.table.state;
  }

  get connected(): boolean {
    return this.ws.connected;
  }

  on(ev: FlowEvent, cb: (arg?: unknown) => void, target?: unknown): void {
    const list = this.listeners.get(ev);
    if (list) list.push({ cb, target });
    else this.listeners.set(ev, [{ cb, target }]);
  }

  off(ev: FlowEvent, cb: (arg?: unknown) => void, target?: unknown): void {
    const list = this.listeners.get(ev);
    if (!list) return;
    const i = list.findIndex((l) => l.cb === cb && l.target === target);
    if (i >= 0) list.splice(i, 1);
  }

  // ---- 动作 ----
  login(uid: string, token: string): void {
    if (this.ws.connected) {
      this.lobby.login(uid, token);
    } else {
      // 未连接（如退出登录后重登）：重连后补发
      this.ws.connect(this.url, () => this.lobby.login(uid, token));
    }
  }

  createRoom(level: number): void {
    this.lobby.createRoom(level);
  }

  joinRoom(roomId: string): void {
    this.lobby.joinRoom(roomId);
  }

  match(): void {
    this.lobby.match();
  }

  logout(): void {
    this.ws.close();
    this.roomId = '';
    this.playerId = 0;
    this.emit('logout');
  }

  // ---- 流程绑定 ----
  private bindFlow(): void {
    this.ws.onMessage(Cmd.S2C_LoginAck, (b) => {
      const ack = decodeLoginAck(b);
      if (ack.code === 0) {
        this.playerId = ack.playerId;
        this.emit('login');
      }
    });
    this.ws.onMessage(Cmd.S2C_CreateRoomAck, (b) => {
      const ack = decodeCreateRoomAck(b);
      if (ack.code === 0) {
        this.roomId = ack.roomId;
        this.emit('roomEntered');
      }
    });
    this.ws.onMessage(Cmd.S2C_MatchAck, (b) => {
      const ack = decodeMatchAck(b);
      if (ack.code === 0) {
        this.roomId = ack.roomId;
        this.emit('roomEntered');
      }
    });
    this.ws.onMessage(Cmd.S2C_RoomState, (b) => {
      const st = decodeRoomState(b);
      if (st.code === 0 && st.roomId) {
        this.roomId = st.roomId;
        this.emit('roomEntered');
      }
    });
    this.ws.onMessage(Cmd.S2C_Error, (b) => {
      const err = decodeError(b);
      this.emit('error', err.msg);
    });
  }

  private emit(ev: FlowEvent, arg?: unknown): void {
    const list = this.listeners.get(ev);
    if (list) {
      for (const l of list) l.cb.call(l.target, arg);
    }
  }
}

// 全局单例：场景组件直接 import 使用。
export const session = new Session(SERVER_URL);
