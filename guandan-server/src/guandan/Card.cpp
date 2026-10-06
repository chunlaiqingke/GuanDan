#include "guandan/Card.h"

#include <algorithm>
#include <random>

namespace guandan::rules {

  std::vector<Card> buildDeck() {
    std::vector<Card> deck;
    deck.reserve(kDeckCards);
    for (int r = kMinRank; r <= kMaxRank; ++r) {
      for (int s = 0; s < 4; ++s) {
        const Card c = makeCard(r, static_cast<Suit>(s));
        deck.push_back(c);
        deck.push_back(c);  // 两副牌
      }
    }
    for (int i = 0; i < 2; ++i) {
      deck.push_back(makeCard(RANK_SMALL_JOKER, Suit::Spade));
      deck.push_back(makeCard(RANK_BIG_JOKER, Suit::Spade));
    }
    return deck;
  }

  void shuffle(std::vector<Card>& deck, uint32_t seed) {
    std::mt19937 rng(seed);
    std::shuffle(deck.begin(), deck.end(), rng);
  }

  std::array<std::vector<Card>, kPlayerCount> deal(const std::vector<Card>& deck) {
    std::array<std::vector<Card>, kPlayerCount> hands;
    if (deck.size() != kDeckCards) return hands;  // 非法 deck，返回空手牌
    for (size_t i = 0; i < deck.size(); ++i) {
      hands[i % kPlayerCount].push_back(deck[i]);
    }
    return hands;
  }

}  // namespace guandan::rules
