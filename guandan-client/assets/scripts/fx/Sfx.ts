// 音效/动画占位钩子（Phase 06 未接 Cocos，先留桩）。

export interface SfxHooks {
  playCard(): void;
  playBomb(): void;
  playWin(): void;
  shakeCountdown(): void;
}

/** 空实现；Cocos 里替换为真实音效/动画。 */
export function noopSfx(): SfxHooks {
  return {
    playCard() {},
    playBomb() {},
    playWin() {},
    shakeCountdown() {},
  };
}
