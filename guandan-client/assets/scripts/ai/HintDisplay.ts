// 提示展示（仅提示展示，不本地算 AI 决策）。

import { teachingText } from '../game/teaching';

export interface HintRecommendation {
  cards: number[];
  text: string;
}

/** 把 Top3 推荐（含理由标签）转为「高亮牌 + 白话讲解」。 */
export function buildHintRecommendation(
  hint: { cards: number[]; reasonTag: number }[],
): HintRecommendation | null {
  if (hint.length === 0) return null;
  const first = hint[0];
  return { cards: first.cards.slice(), text: teachingText(first.reasonTag) };
}
