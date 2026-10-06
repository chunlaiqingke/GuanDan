// 文字牌面渲染（占位 UI，Phase 06 未接美术贴图）。
// 牌编码与服务端一致：rank*4+suit（2..10、11=J、12=Q、13=K、14=A、16=小王、17=大王）。

const SUITS = ['♠', '♥', '♣', '♦'];

const RANK_TEXT: Record<number, string> = {
  2: '2', 3: '3', 4: '4', 5: '5', 6: '6', 7: '7', 8: '8', 9: '9', 10: '10',
  11: 'J', 12: 'Q', 13: 'K', 14: 'A',
};

export function rankOf(card: number): number {
  return card >> 2;
}

export function suitOf(card: number): number {
  return card & 3;
}

/** 单张牌的显示文本，如 "♠A"、"♥10"、"大王"。 */
export function cardLabel(card: number): string {
  const r = rankOf(card);
  if (r === 16) return '小王';
  if (r === 17) return '大王';
  const s = suitOf(card);
  return SUITS[s] + (RANK_TEXT[r] ?? String(r));
}

/** 手牌降序排序（大王在前，同点按花色）。返回新数组。 */
export function sortHand(cards: number[]): number[] {
  return [...cards].sort((a, b) => b - a);
}
