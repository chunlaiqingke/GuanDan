// 大厅/房间控制器：登录、建房、入房、补位机器人（占位 UI）。

import { WsClient } from '../net/WsClient';
import { Cmd } from '../config/protoId';
import {
  encodeAddBotReq,
  encodeCancelMatchReq,
  encodeCreateRoomReq,
  encodeJoinRoomReq,
  encodeLoginReq,
  encodeStartMatchReq,
} from '../net/pb/messages';

export class LobbyController {
  constructor(private readonly ws: WsClient) {}

  login(uid: string, token: string): void {
    this.ws.send(Cmd.C2S_Login, encodeLoginReq({ uid, token }));
  }

  createRoom(level: number): void {
    this.ws.send(Cmd.C2S_CreateRoom, encodeCreateRoomReq({ rule: { level, enableGong: false } }));
  }

  joinRoom(roomId: string): void {
    this.ws.send(Cmd.C2S_JoinRoom, encodeJoinRoomReq({ roomId }));
  }

  addBot(count: number): void {
    this.ws.send(Cmd.C2S_AddBot, encodeAddBotReq({ count }));
  }

  match(): void {
    this.ws.send(Cmd.C2S_StartMatch, encodeStartMatchReq());
  }

  cancelMatch(): void {
    this.ws.send(Cmd.C2S_CancelMatch, encodeCancelMatchReq());
  }
}
