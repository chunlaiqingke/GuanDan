// 牌桌控制器：绑定 WsClient 的 S2C 帧 → 更新 GameState → 驱动 TableView。
// 只读服务端数据，不本地判牌、不本地判定超时。

import { WsClient } from '../net/WsClient';
import { Cmd } from '../config/protoId';
import { GameState } from '../game/GameState';
import { buildHintRecommendation } from '../ai/HintDisplay';
import {
  decodeAutoPlay,
  decodeDeal,
  decodeError,
  decodeGameStart,
  decodeHintAck,
  decodeHostMode,
  decodeMatchAck,
  decodePassResult,
  decodePlayResult,
  decodeRankUpdate,
  decodeRoundEnd,
  decodeTick,
  decodeTurnStart,
  encodeAddBotReq,
  encodeHintReq,
  encodePassReq,
  encodePlayReq,
  encodeSetHostReq,
} from '../net/pb/messages';
import { TableView } from './TableView';

export class TableController {
  readonly state = new GameState();

  constructor(private readonly ws: WsClient, private readonly view: TableView) {
    this.bind();
  }

  // ---- 动作 ----
  play(cards: number[]): void {
    this.ws.send(Cmd.C2S_Play, encodePlayReq({ cards }));
  }

  pass(): void {
    this.ws.send(Cmd.C2S_Pass, encodePassReq());
  }

  hint(): void {
    this.ws.send(Cmd.C2S_Hint, encodeHintReq());
  }

  setHost(host: boolean): void {
    this.ws.send(Cmd.C2S_SetHost, encodeSetHostReq({ host }));
  }

  addBot(count: number): void {
    this.ws.send(Cmd.C2S_AddBot, encodeAddBotReq({ count }));
  }

  // ---- S2C 绑定 ----
  private bind(): void {
    this.ws.onMessage(Cmd.S2C_GameStart, (b) => {
      const m = decodeGameStart(b);
      this.state.applyGameStart(m);
      this.view.onGameStart(m.level, m.firstUid);
    });

    this.ws.onMessage(Cmd.S2C_Deal, (b) => {
      const m = decodeDeal(b);
      this.state.applyDeal(m);
      this.view.onDeal(m.level, m.yourSeat, m.cards);
    });

    this.ws.onMessage(Cmd.S2C_TurnStart, (b) => {
      const m = decodeTurnStart(b);
      this.state.applyTurnStart(m);
      this.view.onTurnStart(m.uid, m.canPass, m.deadlineTs);
    });

    this.ws.onMessage(Cmd.S2C_Tick, (b) => {
      const m = decodeTick(b);
      this.state.applyTick(m);
      this.view.onTick(m.remainMs);
    });

    this.ws.onMessage(Cmd.S2C_PlayResult, (b) => {
      const m = decodePlayResult(b);
      this.state.applyPlayResult(m);
      this.view.onPlay(m.uid, m.cards);
    });

    this.ws.onMessage(Cmd.S2C_PassResult, (b) => {
      const m = decodePassResult(b);
      this.state.applyPassResult(m);
      this.view.onPass(m.uid);
    });

    this.ws.onMessage(Cmd.S2C_AutoPlay, (b) => {
      const m = decodeAutoPlay(b);
      this.state.applyAutoPlay(m);
      this.view.onAutoPlay(m.uid, m.cards, m.isPass, m.reasonTag);
      const rec = buildHintRecommendation([{ cards: m.cards, reasonTag: m.reasonTag }]);
      if (rec) this.view.showTeaching(rec.text);
    });

    this.ws.onMessage(Cmd.S2C_HostMode, (b) => {
      const m = decodeHostMode(b);
      this.state.applyHostMode(m);
      this.view.onHostMode(m.uid, m.isHost);
    });

    this.ws.onMessage(Cmd.S2C_HintAck, (b) => {
      const m = decodeHintAck(b);
      this.state.applyHintAck(m);
      this.view.onHint(m.plays);
      const rec = buildHintRecommendation(m.plays);
      if (rec) this.view.showTeaching(rec.text);
    });

    this.ws.onMessage(Cmd.S2C_RoundEnd, (b) => {
      const m = decodeRoundEnd(b);
      this.state.applyRoundEnd(m);
      this.view.onRoundEnd(m.finishOrder, m.levelUp, m.lastUid);
    });

    this.ws.onMessage(Cmd.S2C_MatchAck, (b) => {
      const m = decodeMatchAck(b);
      this.state.applyMatchAck(m);
      this.view.onMatch(m.roomId, m.level);
    });

    this.ws.onMessage(Cmd.S2C_RankUpdate, (b) => {
      const m = decodeRankUpdate(b);
      this.state.applyRankUpdate(m);
      this.view.onRankUpdate(m.updates);
    });

    this.ws.onMessage(Cmd.S2C_Error, (b) => {
      const m = decodeError(b);
      this.view.onError(m.code, m.msg);
    });
  }
}
