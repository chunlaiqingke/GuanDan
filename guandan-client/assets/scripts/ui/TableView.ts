// 牌桌渲染接口（引擎无关）。Cocos 组件实现此接口即可接上真实渲染。

import { RankInfo } from '../net/pb/messages';

export interface HintItem {
  cards: number[];
  reasonTag: number;
}

export interface TableView {
  onGameStart(level: number, firstUid: number): void;
  onDeal(level: number, yourSeat: number, cards: number[]): void;
  onTurnStart(uid: number, canPass: boolean, deadlineTs: number): void;
  onTick(remainMs: number): void;
  onPlay(uid: number, cards: number[]): void;
  onPass(uid: number): void;
  onAutoPlay(uid: number, cards: number[], isPass: boolean, reasonTag: number): void;
  onHostMode(uid: number, isHost: boolean): void;
  onHint(items: HintItem[]): void;
  onRoundEnd(finishOrder: number[], levelUp: number, lastUid: number): void;
  onMatch(roomId: string, level: number): void;
  onRankUpdate(updates: RankInfo[]): void;
  onError(code: number, msg: string): void;
  showTeaching(text: string): void;
}
