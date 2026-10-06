#pragma once

#include <cstdint>
#include <vector>

#include "guandan/Card.h"

namespace guandan::rules {

  enum class PatternType : uint8_t {
    Invalid = 0,
    Single,          // 单张
    Pair,            // 对子
    Triple,          // 三张
    TripleWithPair,  // 三带二
    Straight,        // 顺子（固定 5 张）
    PairSequence,    // 连对（固定 3 对 = 6 张）
    TripleSequence,  // 飞机（固定 2 组三张 = 6 张）
    StraightFlush,   // 同花顺（固定 5 张）
    Bomb,            // 炸弹（≥4 张同点）
    Rocket,          // 天王炸（4 王）
  };

  struct PatternInfo {
    PatternType type = PatternType::Invalid;
    int mainRank = 0;  // 比较主牌点：单/对/三/炸=点；顺子类=最大牌点；三带二=三张点
    int length = 0;    // 张数
    int pairRank = 0;  // 三带二的对子点（仅三带二用）
  };

  inline bool isBombLevel(PatternType t) {
    return t == PatternType::Bomb || t == PatternType::StraightFlush ||
           t == PatternType::Rocket;
  }

  // 顺子类（比较时按自然牌点，不提升级牌）。
  inline bool isSequence(PatternType t) {
    return t == PatternType::Straight || t == PatternType::PairSequence ||
           t == PatternType::TripleSequence || t == PatternType::StraightFlush;
  }

  // 识别一手牌（含逢人配）。
  PatternInfo classify(const std::vector<Card>& cards, int levelRank);

}  // namespace guandan::rules
