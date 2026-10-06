#include "room/GameTable.h"

#include <algorithm>

namespace guandan::room {

  void GameTable::startRound(const std::array<int64_t, 4>& playerIds, int level, uint32_t seed) {
    auto deck = guandan::rules::buildDeck();
    guandan::rules::shuffle(deck, seed);
    startRound(playerIds, level, guandan::rules::deal(deck));
  }

  void GameTable::startRound(const std::array<int64_t, 4>& playerIds, int level,
                             std::array<std::vector<Card>, 4> hands) {
    playerIds_ = playerIds;
    level_ = level;
    hands_ = std::move(hands);
    finishOrder_.clear();
    phase_ = Phase::Playing;
    leaderSeat_ = 0;
    currentTurn_ = 0;
    lastPlaySeat_ = -1;
    lastPlay_ = {};
    lastPlayCards_.clear();
    passCount_ = 0;
    respondersNeeded_ = 0;
    trickHasPlay_ = false;
  }

  bool GameTable::isOut(int seat) const {
    for (const auto& f : finishOrder_) {
      if (f.seat == seat) return true;
    }
    return false;
  }

  bool GameTable::containsHand(int seat, const std::vector<Card>& cards) const {
    std::vector<Card> tmp = hands_[seat];
    for (Card c : cards) {
      auto it = std::find(tmp.begin(), tmp.end(), c);
      if (it == tmp.end()) return false;
      tmp.erase(it);
    }
    return true;
  }

  void GameTable::removeCards(int seat, const std::vector<Card>& cards) {
    for (Card c : cards) {
      auto it = std::find(hands_[seat].begin(), hands_[seat].end(), c);
      if (it != hands_[seat].end()) hands_[seat].erase(it);
    }
  }

  void GameTable::onHandEmptied(int seat) {
    finishOrder_.push_back({seat, static_cast<int>(finishOrder_.size()) + 1});
    if (finishOrder_.size() >= 3) {
      phase_ = Phase::RoundEnd;
      currentTurn_ = -1;
      leaderSeat_ = -1;
    }
  }

  int GameTable::nextActiveSeat(int seat) const {
    for (int i = 1; i <= 4; ++i) {
      const int s = (seat + i) % 4;
      if (!isOut(s)) return s;
    }
    return -1;
  }

  int GameTable::firstActiveSeat() const {
    for (int s = 0; s < 4; ++s) {
      if (!isOut(s)) return s;
    }
    return -1;
  }

  PlayResult GameTable::play(int seat, const std::vector<Card>& cards) {
    if (phase_ != Phase::Playing) return PlayResult::NotPlaying;
    if (seat != currentTurn_ || isOut(seat)) return PlayResult::NotYourTurn;
    if (cards.empty()) return PlayResult::InvalidCards;
    if (!containsHand(seat, cards)) return PlayResult::CardsNotInHand;

    const auto pat = guandan::rules::classify(cards, level_);
    if (pat.type == guandan::rules::PatternType::Invalid) return PlayResult::InvalidCards;

    if (trickHasPlay_) {
      if (guandan::rules::compare(pat, lastPlay_, level_) != guandan::rules::CompareResult::FirstWins) {
        return PlayResult::CannotBeat;
      }
    }

    removeCards(seat, cards);
    lastPlay_ = pat;
    lastPlaySeat_ = seat;
    lastPlayCards_ = cards;
    trickHasPlay_ = true;
    passCount_ = 0;

    if (hands_[seat].empty()) onHandEmptied(seat);

    if (phase_ == Phase::Playing) {
      respondersNeeded_ = activeCount() - (isOut(seat) ? 0 : 1);
      currentTurn_ = nextActiveSeat(seat);
    }
    return PlayResult::Ok;
  }

  PlayResult GameTable::pass(int seat) {
    if (phase_ != Phase::Playing) return PlayResult::NotPlaying;
    if (seat != currentTurn_ || isOut(seat)) return PlayResult::NotYourTurn;
    if (!trickHasPlay_) return PlayResult::MustLead;

    ++passCount_;
    if (passCount_ >= respondersNeeded_) {
      const int winner = lastPlaySeat_;
      if (isOut(winner)) {
        const int partner = partnerSeat(winner);
        leaderSeat_ = isOut(partner) ? firstActiveSeat() : partner;
      } else {
        leaderSeat_ = winner;
      }
      lastPlay_ = {};
      lastPlaySeat_ = -1;
      lastPlayCards_.clear();
      trickHasPlay_ = false;
      passCount_ = 0;
      respondersNeeded_ = 0;
      currentTurn_ = leaderSeat_;
    } else {
      currentTurn_ = nextActiveSeat(seat);
    }
    return PlayResult::Ok;
  }

  int GameTable::headSeat() const {
    return finishOrder_.empty() ? -1 : finishOrder_[0].seat;
  }

  int GameTable::lastSeat() const {
    if (finishOrder_.size() < 3) return -1;
    int used[4] = {0, 0, 0, 0};
    for (const auto& f : finishOrder_) used[f.seat] = 1;
    for (int s = 0; s < 4; ++s) {
      if (!used[s]) return s;
    }
    return -1;
  }

  int GameTable::levelUp() const {
    if (finishOrder_.size() < 2) return 0;
    return sameTeam(finishOrder_[0].seat, finishOrder_[1].seat) ? 2 : 1;
  }

  std::vector<Card> GameTable::minSingle(int seat) const {
    if (hands_[seat].empty()) return {};
    Card best = hands_[seat][0];
    for (Card c : hands_[seat]) {
      if (guandan::rules::rankOf(c) < guandan::rules::rankOf(best)) best = c;
    }
    return {best};
  }

  std::vector<Tribute> GameTable::computeTribute(const std::vector<FinishInfo>& finishOrder) {
    std::vector<Tribute> out;
    if (finishOrder.size() < 3) return out;
    const int head = finishOrder[0].seat;
    const int second = finishOrder[1].seat;
    const int third = finishOrder[2].seat;
    int used[4] = {0, 0, 0, 0};
    used[head] = used[second] = used[third] = 1;
    int last = -1;
    for (int s = 0; s < 4; ++s) {
      if (!used[s]) last = s;
    }

    if (sameTeam(head, second)) {
      out.push_back({last, head});    // 末游 → 头游
      out.push_back({third, second}); // 三游 → 二游
    } else {
      out.push_back({last, head});    // 末游 → 头游
    }
    return out;
  }

}  // namespace guandan::room

