// 占位文字视图：把牌桌渲染成控制台文本（Phase 06 未接 Cocos，用文字牌面+日志占位）。

import { cardLabel } from './cardDisplay';
import { HintItem, TableView } from './TableView';
import { RankInfo } from '../net/pb/messages';

export class ConsoleView implements TableView {
  onGameStart(level: number, firstUid: number): void {
    console.log(`[开局] 级牌=${level} 首出=player${firstUid}`);
  }

  onDeal(level: number, yourSeat: number, cards: number[]): void {
    console.log(`[发牌] 级牌=${level} 座位=${yourSeat} 手牌=${cards.map(cardLabel).join(' ')}`);
  }

  onTurnStart(uid: number, canPass: boolean, deadlineTs: number): void {
    console.log(`[轮到] player${uid} canPass=${canPass} deadlineTs=${deadlineTs}`);
  }

  onTick(remainMs: number): void {
    console.log(`[倒计时] ${remainMs}ms`);
  }

  onPlay(uid: number, cards: number[]): void {
    console.log(`[出牌] player${uid}: ${cards.map(cardLabel).join(' ')}`);
  }

  onPass(uid: number): void {
    console.log(`[过牌] player${uid}`);
  }

  onAutoPlay(uid: number, cards: number[], isPass: boolean, reasonTag: number): void {
    console.log(`[自动] player${uid} ${isPass ? '过' : cards.map(cardLabel).join(' ')} tag=${reasonTag}`);
  }

  onHostMode(uid: number, isHost: boolean): void {
    console.log(`[托管] player${uid} ${isHost ? '托管中' : '取消托管'}`);
  }

  onHint(items: HintItem[]): void {
    console.log(`[提示] ${items.map((it) => it.cards.map(cardLabel).join(' ')).join(' | ')}`);
  }

  onRoundEnd(finishOrder: number[], levelUp: number, lastUid: number): void {
    console.log(`[结算] 名次=${finishOrder.join('>')} 升${levelUp}级 末游=player${lastUid}`);
  }

  onMatch(roomId: string, level: number): void {
    console.log(`[匹配成功] room=${roomId} 级牌=${level}`);
  }

  onRankUpdate(updates: RankInfo[]): void {
    for (const u of updates) {
      console.log(`[段位] player${u.playerId} ${u.oldRating}→${u.newRating} (${u.delta >= 0 ? '+' : ''}${u.delta}) ${u.tier}`);
    }
  }

  onError(code: number, msg: string): void {
    console.log(`[错误] ${code} ${msg}`);
  }

  showTeaching(text: string): void {
    if (text) console.log(`[教学] ${text}`);
  }
}
