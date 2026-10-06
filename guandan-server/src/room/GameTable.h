#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "guandan/Card.h"
#include "guandan/Compare.h"
#include "guandan/Pattern.h"

namespace guandan::room {

  using guandan::rules::Card;

  enum class Phase : uint8_t {
    Waiting,   // 未开局
    Playing,   // 出牌中
    RoundEnd,  // 一盘结束（3 人出完）
  };

  enum class PlayResult : uint8_t {
    Ok,
    NotYourTurn,
    NotPlaying,
    InvalidCards,    // 牌型非法
    CardsNotInHand,  // 手牌不含这些牌
    CannotBeat,      // 不能压过上一手
    MustLead,        // 领出不能过
  };

  // 进贡信息（Phase 03 仅纯计算 + 单测，不接循环）。
  struct Tribute {
    int fromSeat = -1;
    int toSeat = -1;
  };

  struct FinishInfo {
    int seat = -1;
    int rank = 0;  // 1=头游 2=二游 3=三游
  };

  inline int partnerSeat(int seat) { return (seat + 2) % 4; }
  inline bool sameTeam(int a, int b) { return (a & 1) == (b & 1); }

  // 单盘状态机：发牌 → 出牌循环 → 头/二/三游 → 升级。纯逻辑，不含网络/定时。
  class GameTable {
   public:
    GameTable() = default;

    // 开局（洗牌 + 发牌）。
    void startRound(const std::array<int64_t, 4>& playerIds, int level, uint32_t seed);

    // 开局（直接给定手牌，供测试）。
    void startRound(const std::array<int64_t, 4>& playerIds, int level,
                    std::array<std::vector<Card>, 4> hands);

    Phase phase() const { return phase_; }
    int level() const { return level_; }
    int currentTurn() const { return currentTurn_; }
    int leaderSeat() const { return leaderSeat_; }
    int64_t playerId(int seat) const { return playerIds_[seat]; }

    bool isOut(int seat) const;
    int handCount(int seat) const { return static_cast<int>(hands_[seat].size()); }
    const std::vector<Card>& hand(int seat) const { return hands_[seat]; }

    const std::vector<Card>& lastPlayCards() const { return lastPlayCards_; }
    const guandan::rules::PatternInfo& lastPlay() const { return lastPlay_; }
    int lastPlaySeat() const { return lastPlaySeat_; }
    bool canPass() const { return trickHasPlay_; }

    PlayResult play(int seat, const std::vector<Card>& cards);
    PlayResult pass(int seat);

    const std::vector<FinishInfo>& finishOrder() const { return finishOrder_; }
    int headSeat() const;
    int lastSeat() const;

    // 升级数（0/1/2）。
    int levelUp() const;

    // 首出超时：最小单张。
    std::vector<Card> minSingle(int seat) const;

    // 进贡纯计算（finishOrder 含头/二/三游，第 4 人即末游）。
    static std::vector<Tribute> computeTribute(const std::vector<FinishInfo>& finishOrder);

   private:
    bool containsHand(int seat, const std::vector<Card>& cards) const;
    void removeCards(int seat, const std::vector<Card>& cards);
    void onHandEmptied(int seat);
    int nextActiveSeat(int seat) const;
    int firstActiveSeat() const;
    int activeCount() const { return 4 - static_cast<int>(finishOrder_.size()); }

    Phase phase_ = Phase::Waiting;
    int level_ = 2;
    std::array<int64_t, 4> playerIds_{};
    std::array<std::vector<Card>, 4> hands_{};

    int currentTurn_ = -1;
    int leaderSeat_ = -1;
    int lastPlaySeat_ = -1;
    guandan::rules::PatternInfo lastPlay_;
    std::vector<Card> lastPlayCards_;
    int passCount_ = 0;
    int respondersNeeded_ = 0;
    bool trickHasPlay_ = false;

    std::vector<FinishInfo> finishOrder_;
  };

}  // namespace guandan::room
