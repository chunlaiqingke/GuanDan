// WebSocket 客户端封装（用引擎内置/全局 WebSocket，浏览器与 Cocos 原生 jsb 通用）。
// 职责：连接 / 二进制帧收发 / 心跳 / 指数退避重连 / 按 cmd 分发消息。

import { Cmd } from '../config/protoId';
import { FRAME_HEADER_SIZE, FRAME_VERSION, decodeHeader, encodeFrame } from './frame';
import { encodeHeartbeatReq, encodeReconnectReq } from './pb/messages';

export type MessageHandler = (body: Uint8Array) => void;

const HEARTBEAT_INTERVAL_MS = 5000;
const RECONNECT_BASE_MS = 1000;
const RECONNECT_MAX_MS = 8000;

export class WsClient {
  private ws: WebSocket | null = null;
  private url = '';
  private handlers = new Map<number, MessageHandler[]>();

  private manualClose = false;
  private reconnectAttempts = 0;
  private reconnectTimer: ReturnType<typeof setTimeout> | null = null;
  private heartbeatTimer: ReturnType<typeof setInterval> | null = null;

  private onOpenCb: (() => void) | null = null;
  private onCloseCb: (() => void) | null = null;
  private onErrorCb: ((err: unknown) => void) | null = null;

  // 重连上下文（登录成功后由业务填充）
  private playerId = 0;
  private roomId = '';

  get connected(): boolean {
    return this.ws !== null && this.ws.readyState === WebSocket.OPEN;
  }

  setReconnectContext(playerId: number, roomId: string): void {
    this.playerId = playerId;
    this.roomId = roomId;
  }

  connect(url: string, onOpen?: () => void, onClose?: () => void): void {
    this.url = url;
    this.manualClose = false;
    this.onOpenCb = onOpen ?? null;
    this.onCloseCb = onClose ?? null;

    const ws = new WebSocket(url);
    ws.binaryType = 'arraybuffer'; // 二进制帧
    this.ws = ws;

    ws.onopen = () => {
      this.reconnectAttempts = 0;
      this.startHeartbeat();
      // 重连成功后补发 C2S_Reconnect（首次连接 playerId==0 不会触发）
      if (this.playerId > 0) this.sendReconnect();
      if (this.onOpenCb) this.onOpenCb();
    };

    ws.onmessage = (ev: MessageEvent) => {
      const data = ev.data;
      if (!(data instanceof ArrayBuffer)) return; // 只用二进制帧
      this.dispatch(new Uint8Array(data));
    };

    ws.onclose = () => {
      this.stopHeartbeat();
      this.ws = null;
      if (this.onCloseCb) this.onCloseCb();
      if (!this.manualClose) this.scheduleReconnect();
    };

    ws.onerror = (err: unknown) => {
      if (this.onErrorCb) this.onErrorCb(err);
    };
  }

  setOnError(cb: (err: unknown) => void): void {
    this.onErrorCb = cb;
  }

  /** 发送一条业务消息：cmd + protobuf body -> 完整帧。 */
  send(cmd: number, body: Uint8Array): void {
    if (!this.connected) return;
    const frame = encodeFrame(cmd, body);
    this.ws!.send(frame);
  }

  /** 注册 cmd 回调。 */
  onMessage(cmd: number, cb: MessageHandler): void {
    const list = this.handlers.get(cmd);
    if (list) list.push(cb);
    else this.handlers.set(cmd, [cb]);
  }

  close(): void {
    this.manualClose = true;
    this.clearReconnectTimer();
    this.stopHeartbeat();
    if (this.ws) this.ws.close();
    this.ws = null;
  }

  private dispatch(frame: Uint8Array): void {
    const hdr = decodeHeader(frame);
    if (!hdr) return;
    if (hdr.ver !== FRAME_VERSION) return;
    if (hdr.len !== frame.byteLength - FRAME_HEADER_SIZE) return;
    const body = frame.slice(FRAME_HEADER_SIZE);
    const list = this.handlers.get(hdr.cmd);
    if (list) {
      for (const cb of list) cb(body);
    }
  }

  private startHeartbeat(): void {
    this.stopHeartbeat();
    this.heartbeatTimer = setInterval(() => {
      if (this.connected) {
        this.send(Cmd.C2S_Heartbeat, encodeHeartbeatReq({ ts: Date.now() }));
      }
    }, HEARTBEAT_INTERVAL_MS);
  }

  private stopHeartbeat(): void {
    if (this.heartbeatTimer !== null) {
      clearInterval(this.heartbeatTimer);
      this.heartbeatTimer = null;
    }
  }

  private scheduleReconnect(): void {
    const delay = Math.min(RECONNECT_BASE_MS * 2 ** this.reconnectAttempts, RECONNECT_MAX_MS);
    this.reconnectAttempts++;
    this.reconnectTimer = setTimeout(() => {
      this.connect(this.url, this.onOpenCb ?? undefined, this.onCloseCb ?? undefined);
    }, delay);
  }

  private clearReconnectTimer(): void {
    if (this.reconnectTimer !== null) {
      clearTimeout(this.reconnectTimer);
      this.reconnectTimer = null;
    }
  }

  /** 重连成功后由业务调用：携带房间上下文重连。 */
  sendReconnect(): void {
    if (this.connected && this.playerId > 0) {
      this.send(Cmd.C2S_Reconnect, encodeReconnectReq({ playerId: this.playerId, roomId: this.roomId }));
    }
  }
}
