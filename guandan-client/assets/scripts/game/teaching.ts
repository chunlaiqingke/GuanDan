// 教学文案：reason_tag -> 白话讲解。
// 标签枚举见 guandan-server/src/guandan/ai/Strategy.h 的 ReasonTag。
export function teachingText(tag: number): string {
  switch (tag) {
    case 1: return '这手牌能一次出完，直接走！';
    case 2: return '用最小的牌压住，保留大牌';
    case 3: return '压住上一手';
    case 4: return '先过一手，保留炸弹到关键回合';
    case 5: return '让牌给队友，队友领先';
    case 6: return '对手快出完了，用炸弹堵住';
    default: return '';
  }
}
