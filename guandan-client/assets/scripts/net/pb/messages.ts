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
  isBot: boolean;
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
  const out: RoomPlayer = { playerId: 0, name: '', seat: 0, isBot: false };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.playerId = r.readVarint();
    else if (fieldNo === 2) out.name = r.readString();
    else if (fieldNo === 3) out.seat = r.readVarint();
    else if (fieldNo === 4) out.isBot = r.readVarint() !== 0;
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

// =====================================================================
// Phase 03 对战流消息
// =====================================================================

export interface SetHostReq {
  host: boolean;
}

export interface TurnStart {
  uid: number;
  deadlineTs: number;
  canPass: boolean;
}

export interface Tick {
  remainMs: number;
}

export interface AutoPlay {
  uid: number;
  cards: number[];
  isPass: boolean;
}

export interface HostMode {
  uid: number;
  isHost: boolean;
}

export interface PlayReq {
  cards: number[];
}

export interface Deal {
  level: number;
  yourSeat: number;
  cards: number[];
  seats: number[];
}

export interface RoundEnd {
  finishOrder: number[];
  levelUp: number;
  lastUid: number;
}

export interface GameStart {
  level: number;
  firstUid: number;
}

export interface PlayResultMsg {
  uid: number;
  cards: number[];
}

export interface PassResultMsg {
  uid: number;
}

// 读取 packed repeated varint（protoc 对 proto3 标量 repeated 默认打包）。
function readPackedVarints(b: Uint8Array): number[] {
  const r = new Reader(b);
  const out: number[] = [];
  while (!r.eof) out.push(r.readVarint());
  return out;
}

// ---- 编码 ----

export function encodeSetHostReq(m: SetHostReq): Uint8Array {
  const w = new Writer();
  w.writeBool(1, m.host);
  return w.finish();
}

export function encodePlayReq(m: PlayReq): Uint8Array {
  const w = new Writer();
  for (const c of m.cards) w.writeInt32(1, c);
  return w.finish();
}

export function encodePassReq(): Uint8Array {
  return new Writer().finish();
}

// ---- 解码 ----

export function decodeTurnStart(b: Uint8Array): TurnStart {
  const r = new Reader(b);
  const out: TurnStart = { uid: 0, deadlineTs: 0, canPass: false };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.uid = r.readVarint();
    else if (fieldNo === 2) out.deadlineTs = r.readVarint();
    else if (fieldNo === 3) out.canPass = r.readVarint() !== 0;
    else r.skip(wt);
  }
  return out;
}

export function decodeTick(b: Uint8Array): Tick {
  const r = new Reader(b);
  const out: Tick = { remainMs: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.remainMs = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

export function decodeAutoPlay(b: Uint8Array): AutoPlay {
  const r = new Reader(b);
  const out: AutoPlay = { uid: 0, cards: [], isPass: false };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.uid = r.readVarint();
    else if (fieldNo === 2) out.cards.push(...readPackedVarints(r.readBytes()));
    else if (fieldNo === 3) out.isPass = r.readVarint() !== 0;
    else r.skip(wt);
  }
  return out;
}

export function decodeHostMode(b: Uint8Array): HostMode {
  const r = new Reader(b);
  const out: HostMode = { uid: 0, isHost: false };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.uid = r.readVarint();
    else if (fieldNo === 2) out.isHost = r.readVarint() !== 0;
    else r.skip(wt);
  }
  return out;
}

export function decodeDeal(b: Uint8Array): Deal {
  const r = new Reader(b);
  const out: Deal = { level: 0, yourSeat: -1, cards: [], seats: [] };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.level = r.readVarint();
    else if (fieldNo === 2) out.yourSeat = r.readVarint();
    else if (fieldNo === 3) out.cards.push(...readPackedVarints(r.readBytes()));
    else if (fieldNo === 4) out.seats.push(...readPackedVarints(r.readBytes()));
    else r.skip(wt);
  }
  return out;
}

export function decodeRoundEnd(b: Uint8Array): RoundEnd {
  const r = new Reader(b);
  const out: RoundEnd = { finishOrder: [], levelUp: 0, lastUid: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.finishOrder.push(...readPackedVarints(r.readBytes()));
    else if (fieldNo === 2) out.levelUp = r.readVarint();
    else if (fieldNo === 3) out.lastUid = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

export function decodeGameStart(b: Uint8Array): GameStart {
  const r = new Reader(b);
  const out: GameStart = { level: 0, firstUid: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.level = r.readVarint();
    else if (fieldNo === 2) out.firstUid = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

export function decodePlayResult(b: Uint8Array): PlayResultMsg {
  const r = new Reader(b);
  const out: PlayResultMsg = { uid: 0, cards: [] };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.uid = r.readVarint();
    else if (fieldNo === 2) out.cards.push(...readPackedVarints(r.readBytes()));
    else r.skip(wt);
  }
  return out;
}

export function decodePassResult(b: Uint8Array): PassResultMsg {
  const r = new Reader(b);
  const out: PassResultMsg = { uid: 0 };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.uid = r.readVarint();
    else r.skip(wt);
  }
  return out;
}

// =====================================================================
// Phase 04：AI Bot 补位 + 提示出牌
// =====================================================================

export interface AddBotReq {
  count: number;
}

export interface HintAck {
  code: number;
  plays: number[][]; // 每个元素是一手牌的 cards
}

export function encodeAddBotReq(m: AddBotReq): Uint8Array {
  const w = new Writer();
  w.writeInt32(1, m.count);
  return w.finish();
}

export function encodeHintReq(): Uint8Array {
  return new Writer().finish();
}

function decodeHintPlay(b: Uint8Array): number[] {
  const r = new Reader(b);
  const cards: number[] = [];
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) cards.push(...readPackedVarints(r.readBytes()));
    else r.skip(wt);
  }
  return cards;
}

export function decodeHintAck(b: Uint8Array): HintAck {
  const r = new Reader(b);
  const out: HintAck = { code: 0, plays: [] };
  let tag: [number, number] | null;
  while ((tag = r.readTag()) !== null) {
    const [fieldNo, wt] = tag;
    if (fieldNo === 1) out.code = r.readVarint();
    else if (fieldNo === 2) out.plays.push(decodeHintPlay(r.readBytes()));
    else r.skip(wt);
  }
  return out;
}


