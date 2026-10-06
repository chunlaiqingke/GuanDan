#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace guandan::rules {

  // 花色
  enum class Suit : uint8_t {
    Spade = 0,    // 黑桃
    Heart = 1,    // 红桃
    Club = 2,     // 梅花
    Diamond = 3,  // 方块
  };

  // 牌点（自然值）。2..10 即面值，J/Q/K/A 用 11..14，王单独编码。
  enum Rank : int {
    RANK_2 = 2,
    RANK_3 = 3,
    RANK_4 = 4,
    RANK_5 = 5,
    RANK_6 = 6,
    RANK_7 = 7,
    RANK_8 = 8,
    RANK_9 = 9,
    RANK_10 = 10,
    RANK_J = 11,
    RANK_Q = 12,
    RANK_K = 13,
    RANK_A = 14,
    RANK_SMALL_JOKER = 16,  // 小王
    RANK_BIG_JOKER = 17,    // 大王
  };

  constexpr int kMinRank = RANK_2;
  constexpr int kMaxRank = RANK_A;
  constexpr int kDeckCards = 108;
  constexpr int kHandCards = 27;
  constexpr int kPlayerCount = 4;

  // 单张牌编码：rank * 4 + suit。
  using Card = int32_t;

  inline int rankOf(Card c) { return static_cast<int>(c >> 2); }
  inline Suit suitOf(Card c) { return static_cast<Suit>(c & 3); }
  inline Card makeCard(int rank, Suit suit) {
    return static_cast<Card>((rank << 2) | static_cast<int>(suit));
  }
  inline bool isJoker(Card c) { return rankOf(c) >= RANK_SMALL_JOKER; }
  inline bool isBigJoker(Card c) { return rankOf(c) == RANK_BIG_JOKER; }
  inline bool isSmallJoker(Card c) { return rankOf(c) == RANK_SMALL_JOKER; }

  // 逢人配：红桃当前级牌（可配除王外任意牌）。
  inline bool isWildcard(Card c, int levelRank) {
    return suitOf(c) == Suit::Heart && rankOf(c) == levelRank;
  }

  // 两副牌 108 张。
  std::vector<Card> buildDeck();

  // 用指定种子洗牌（std::mt19937），可复现。
  void shuffle(std::vector<Card>& deck, uint32_t seed);

  // 发牌：每人 27 张（round-robin）。非法 deck 返回空手牌。
  std::array<std::vector<Card>, kPlayerCount> deal(const std::vector<Card>& deck);

}  // namespace guandan::rules
