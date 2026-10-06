// 牌桌状态模型（只读服务端数据）：订阅 S2C 帧，更新本地视图。
// 客户端不本地判牌、不本地判定超时，一切以服务端推送为准。

import {
  AutoPlay,
  Deal,
  GameStart,
  HintAck,
  HostMode,
  PlayResultMsg,
  RoomState,
  RoundEnd,
  Tick,
  TurnStart,
} from '../net/pb/messages';

export interface SeatView {
  playerId: number;
  handCount: number; // 其余座位只显示张数
  isHost: boolean;
  isOut: boolean;
}

export class GameState {
  level = 2;
  yourSeat = -1;
  hand: number[] = [];
  seats: SeatView[] = [];
  currentUid = 0;
  canPass = false;
  remainMs = 0;
  lastPlay: PlayResultMsg | null = null; // 桌面当前最大一手
  finishOrder: number[] = [];
  levelUp = 0;
  lastUid = 0;
  hint: number[][] = [];
  botUids = new Set<number>();

  applyGameStart(m: GameStart): void {
    this.level = m.level;
    this.currentUid = m.firstUid;
  }

  applyDeal(m: Deal): void {
    this.level = m.level;
    this.yourSeat = m.yourSeat;
    this.hand = m.cards.slice();
    this.seats = m.seats.map((playerId) => ({
      playerId,
      handCount: 27,
      isHost: false,
      isOut: false,
    }));
  }

  applyTurnStart(m: TurnStart): void {
    this.currentUid = m.uid;
    this.canPass = m.canPass;
    this.remainMs = 0; // 由 S2C_Tick 更新
  }

  applyTick(m: Tick): void {
    this.remainMs = m.remainMs;
  }

  applyPlayResult(m: PlayResultMsg): void {
    this.lastPlay = m;
    const seat = this.seatOf(m.uid);
    if (seat >= 0) {
      this.seats[seat].handCount -= m.cards.length;
      if (this.seats[seat].handCount < 0) this.seats[seat].handCount = 0;
    }
    if (m.uid === this.currentUid && seat === this.yourSeat) {
      this.removeFromHand(m.cards);
    }
  }

  applyPassResult(_m: { uid: number }): void {
    // 过牌仅推进回合；回合推进由 S2C_TurnStart 驱动。
  }

  applyAutoPlay(m: AutoPlay): void {
    if (m.isPass) this.applyPassResult({ uid: m.uid });
    else this.applyPlayResult({ uid: m.uid, cards: m.cards });
  }

  applyHostMode(m: HostMode): void {
    const seat = this.seatOf(m.uid);
    if (seat >= 0) this.seats[seat].isHost = m.isHost;
  }

  applyRoundEnd(m: RoundEnd): void {
    this.finishOrder = m.finishOrder.slice();
    this.levelUp = m.levelUp;
    this.lastUid = m.lastUid;
    for (const uid of m.finishOrder) {
      const seat = this.seatOf(uid);
      if (seat >= 0) this.seats[seat].isOut = true;
    }
  }

  applyRoomState(m: RoomState): void {
    for (const p of m.players) {
      if (p.isBot) this.botUids.add(p.playerId);
      else this.botUids.delete(p.playerId);
    }
  }

  applyHintAck(m: HintAck): void {
    this.hint = m.plays.map((cards) => cards.slice());
  }

  isBot(uid: number): boolean {
    return this.botUids.has(uid);
  }

  seatOf(uid: number): number {
    return this.seats.findIndex((s) => s.playerId === uid);
  }

  get mySeat(): SeatView | null {
    return this.yourSeat >= 0 && this.yourSeat < this.seats.length
      ? this.seats[this.yourSeat]
      : null;
  }

  get isMyTurn(): boolean {
    return this.mySeat !== null && this.mySeat.playerId === this.currentUid;
  }

  private removeFromHand(cards: number[]): void {
    for (const c of cards) {
      const i = this.hand.indexOf(c);
      if (i >= 0) this.hand.splice(i, 1);
    }
  }
}
