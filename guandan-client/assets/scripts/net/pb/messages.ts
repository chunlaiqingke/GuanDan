// Phase 01 消息类型 + 编解码，与 guandan-server/src/proto/messages.proto 字段一致。
// 字段名用 camelCase，编解码时映射到 proto 的 snake_case 字段号。

import { Writer, Reader } from './wire';

// ---- 类型定义 ----

export interface LoginReq {
  uid: string;
  token: string;
}

export interface LoginAck {
  code: number;
  playerId: number;
  msg: string;
}

export interface RoomRuleConfig {
  level: number;
  enableGong: boolean;
}

export interface CreateRoomReq {
  rule: RoomRuleConfig;
}

export interface CreateRoomAck {
  code: number;
  roomId: string;
  msg: string;
}

export interface JoinRoomReq {
  roomId: string;
}

export interface RoomPlayer {
  playerId: number;
  name: string;
  seat: number;
}

export interface RoomState {
  code: number;
  roomId: string;
  players: RoomPlayer[];
  msg: string;
}

export interface ReconnectReq {
  playerId: number;
  roomId: string;
}

export interface HeartbeatReq {
  ts: number;
}

export interface HeartbeatAck {
  ts: number;
}

export interface ErrorMsg {
  code: number;
  msg: string;
}

// ---- 编码 ----

export function encodeLoginReq(m: LoginReq): Uint8Array {
  const w = new Writer();
  w.writeString(1, m.uid);
  w.writeString(2, m.token);
  return w.finish();
}

export function encodeCreateRoomReq(m: CreateRoomReq): Uint8Array {
  const inner = new Writer();
  inner.writeInt32(1, m.rule.level);
  inner.writeBool(2, m.rule.enableGong);
  const w = new Writer();
  w.writeMessage(1, inner.finish());
  return w.finish();
}

export function encodeJoinRoomReq(m: JoinRoomReq): Uint8Array {
  const w = new Writer();
  w.writeString(1, m.roomId);
  return w.finish();
}

export function encodeReconnectReq(m: ReconnectReq): Uint8Array {
  const w = new Writer();
  w.writeInt64(1, m.playerId);
  w.writeString(2, m.roomId);
  return w.finish();
}

export function encodeHeartbeatReq(m: HeartbeatReq): Uint8Array {
  const w = new Writer();
  w.writeInt64(1, m.ts);
  return w.finish();
}

// ---- 解码 ----

export function decodeLoginAck(b: Uint8Array): LoginAck {
  const r = new Reader(b);
  const out: LoginAck = { code: 0, playerId: 0, msg: '' };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.code = r.readVarint();
    else if (fieldNo === 2) out.playerId = r.readVarint();
    else if (fieldNo === 3) out.msg = r.readString();
    else r.skip(wt);
  }
  return out;
}

export function decodeCreateRoomAck(b: Uint8Array): CreateRoomAck {
  const r = new Reader(b);
  const out: CreateRoomAck = { code: 0, roomId: '', msg: '' };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.code = r.readVarint();
    else if (fieldNo === 2) out.roomId = r.readString();
    else if (fieldNo === 3) out.msg = r.readString();
    else r.skip(wt);
  }
  return out;
}

function decodeRoomPlayer(b: Uint8Array): RoomPlayer {
  const r = new Reader(b);
  const out: RoomPlayer = { playerId: 0, name: '', seat: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.playerId = r.readVarint();
    else if (fieldNo === 2) out.name = r.readString();
    else if (fieldNo === 3) out.seat = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

export function decodeRoomState(b: Uint8Array): RoomState {
  const r = new Reader(b);
  const out: RoomState = { code: 0, roomId: '', players: [], msg: '' };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.code = r.readVarint();
    else if (fieldNo === 2) out.roomId = r.readString();
    else if (fieldNo === 3) out.players.push(decodeRoomPlayer(r.readBytes()));
    else if (fieldNo === 4) out.msg = r.readString();
    else r.skip(wt);
  }
  return out;
}

export function decodeHeartbeatAck(b: Uint8Array): HeartbeatAck {
  const r = new Reader(b);
  const out: HeartbeatAck = { ts: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.ts = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

export function decodeError(b: Uint8Array): ErrorMsg {
  const r = new Reader(b);
  const out: ErrorMsg = { code: 0, msg: '' };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.code = r.readVarint();
    else if (fieldNo === 2) out.msg = r.readString();
    else r.skip(wt);
  }
  return out;
}
