#include "proto/Dispatcher.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "messages.pb.h"
#include "net/Frame.h"
#include "proto/Cmd.h"
#include "guandan/ai/Strategy.h"
#include "guandan/Card.h"
#include "util/Log.h"
#include "util/Time.h"

namespace guandan::proto {

  namespace {

    constexpr int64_t kTurnTimeoutMs = 15000;
    constexpr int64_t kTickWindowMs = 5000;  // 最后 5s 推 Tick

    template <typename M>
    std::string serialize(const M& m) {
      std::string out;
      m.SerializeToString(&out);
      return out;
    }

  }  // namespace

  Dispatcher::Dispatcher(room::RoomManager& rooms) : rooms_(rooms) {}

  void Dispatcher::onBinary(net::WsSession& s, const uint8_t* data, size_t len) {
    net::FrameHeader hdr;
    if (!net::decodeHeader(data, len, hdr)) {
      GD_LOG_WARN("bad frame header, len={}", len);
      sendError(s, 400, "bad frame header");
      return;
    }
    if (hdr.ver != net::kVersion) {
      GD_LOG_WARN("unsupported version {}", hdr.ver);
      sendError(s, 400, "unsupported version");
      return;
    }
    if (hdr.len != len - net::kFrameHeaderSize) {
      GD_LOG_WARN("frame length mismatch hdr={} actual={}", hdr.len, len - net::kFrameHeaderSize);
      sendError(s, 400, "frame length mismatch");
      return;
    }

    const std::string body(reinterpret_cast<const char*>(data + net::kFrameHeaderSize), hdr.len);

    switch (static_cast<Cmd>(hdr.cmd)) {
      case Cmd::C2S_Login: handleLogin(s, body); break;
      case Cmd::C2S_CreateRoom: handleCreateRoom(s, body); break;
      case Cmd::C2S_JoinRoom: handleJoinRoom(s, body); break;
      case Cmd::C2S_Heartbeat: handleHeartbeat(s, body); break;
      case Cmd::C2S_Reconnect: handleReconnect(s, body); break;
      case Cmd::C2S_Play: handlePlay(s, body); break;
      case Cmd::C2S_Pass: handlePass(s, body); break;
      case Cmd::C2S_SetHost: handleSetHost(s, body); break;
      case Cmd::C2S_AddBot: handleAddBot(s, body); break;
      case Cmd::C2S_Hint: handleHint(s, body); break;
      case Cmd::C2S_StartMatch: handleStartMatch(s, body); break;
      case Cmd::C2S_CancelMatch: handleCancelMatch(s, body); break;
      default:
        GD_LOG_WARN("unknown cmd {}", hdr.cmd);
        sendError(s, 404, "unknown cmd");
        break;
    }
  }

  void Dispatcher::onDisconnect(net::WsSession& s) {
    GD_LOG_INFO("disconnect playerId={} roomId={}", s.playerId(), s.roomId());
    auto it = sessions_.find(s.playerId());
    if (it != sessions_.end() && it->second == &s) sessions_.erase(it);

    // 退出匹配队列
    matchQueue_.erase(
        std::remove_if(matchQueue_.begin(), matchQueue_.end(),
                       [&](const auto& p) { return p.first == s.playerId(); }),
        matchQueue_.end());

    // 断线进入托管态
    if (!s.roomId().empty()) {
      room::Room* room = rooms_.findMutable(s.roomId());
      if (room && room->started) {
        const int seat = room::seatOf(*room, s.playerId());
        if (seat >= 0 && !room->hostMode[seat]) {
          room->hostMode[seat] = true;
          HostMode hm;
          hm.set_uid(s.playerId());
          hm.set_is_host(true);
          broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_HostMode), serialize(hm));
        }
      }
    }
  }

  void Dispatcher::sendError(net::WsSession& s, int code, const std::string& msg) {
    ErrorMsg err;
    err.set_code(code);
    err.set_msg(msg);
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_Error), serialize(err)));
  }

  void Dispatcher::sendTo(int64_t playerId, uint16_t cmd, const std::string& body) {
    auto it = sessions_.find(playerId);
    if (it != sessions_.end()) {
      it->second->sendBinary(net::encodeFrame(cmd, body));
    }
  }

  void Dispatcher::broadcastRoom(const room::Room& room, uint16_t cmd, const std::string& body) {
    for (const auto& p : room.players) sendTo(p.playerId, cmd, body);
  }

  void Dispatcher::handleLogin(net::WsSession& s, const std::string& body) {
    LoginReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse LoginReq failed");
      return;
    }
    const int64_t playerId = nextPlayerId_++;
    s.setPlayerId(playerId);
    sessions_[playerId] = &s;
    LoginAck ack;
    ack.set_code(0);
    ack.set_player_id(playerId);
    ack.set_msg("ok");
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_LoginAck), serialize(ack)));
    GD_LOG_INFO("login uid={} -> playerId={}", req.uid(), playerId);
  }

  void Dispatcher::handleCreateRoom(net::WsSession& s, const std::string& body) {
    CreateRoomReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse CreateRoomReq failed");
      return;
    }
    const std::string roomId = rooms_.createRoom(s.playerId());
    s.setRoomId(roomId);
    rooms_.joinRoom(roomId, s.playerId());  // 房主自身入座 seat 0
    if (room::Room* room = rooms_.findMutable(roomId)) {
      const int lv = req.rule().level();
      room->level = (lv >= 2 && lv <= 14) ? lv : 2;
    }
    CreateRoomAck ack;
    ack.set_code(0);
    ack.set_room_id(roomId);
    ack.set_msg("ok");
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_CreateRoomAck), serialize(ack)));
    GD_LOG_INFO("create room {} by playerId={}", roomId, s.playerId());
  }

  void Dispatcher::handleJoinRoom(net::WsSession& s, const std::string& body) {
    JoinRoomReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse JoinRoomReq failed");
      return;
    }
    if (!rooms_.joinRoom(req.room_id(), s.playerId())) {
      sendError(s, 404, "room not found or full");
      return;
    }
    s.setRoomId(req.room_id());

    RoomState state;
    state.set_code(0);
    state.set_room_id(req.room_id());
    if (const room::Room* room = rooms_.find(req.room_id())) {
      for (const auto& p : room->players) {
        auto* rp = state.add_players();
        rp->set_player_id(p.playerId);
        rp->set_name(p.name);
        rp->set_seat(p.seat);
        rp->set_is_bot(p.isBot);
      }
    }
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_RoomState), serialize(state)));
    GD_LOG_INFO("join room {} playerId={}", req.room_id(), s.playerId());

    maybeStartGame(req.room_id());
  }

  void Dispatcher::handleHeartbeat(net::WsSession& s, const std::string& body) {
    HeartbeatReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse HeartbeatReq failed");
      return;
    }
    s.markHeartbeat(util::nowMs());
    HeartbeatAck ack;
    ack.set_ts(req.ts());
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_HeartbeatAck), serialize(ack)));
  }

  void Dispatcher::handleReconnect(net::WsSession& s, const std::string& body) {
    ReconnectReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse ReconnectReq failed");
      return;
    }
    s.setPlayerId(req.player_id());
    s.setRoomId(req.room_id());
    sessions_[req.player_id()] = &s;

    RoomState state;
    state.set_code(0);
    state.set_room_id(req.room_id());
    const room::Room* room = rooms_.find(req.room_id());
    if (room) {
      for (const auto& p : room->players) {
        auto* rp = state.add_players();
        rp->set_player_id(p.playerId);
        rp->set_name(p.name);
        rp->set_seat(p.seat);
        rp->set_is_bot(p.isBot);
      }
    } else {
      state.set_code(404);
      state.set_msg("room not found");
    }
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_RoomState), serialize(state)));
    GD_LOG_INFO("reconnect playerId={} roomId={}", s.playerId(), s.roomId());

    // 重连成功：补发手牌 + 当前回合
    if (room && room->started) {
      const int seat = room::seatOf(*room, req.player_id());
      if (seat >= 0) {
        Deal deal;
        deal.set_level(room->table.level());
        deal.set_your_seat(seat);
        for (guandan::rules::Card c : room->table.hand(seat)) deal.add_cards(c);
        for (const auto& p : room->players) deal.add_seats(p.playerId);
        s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_Deal), serialize(deal)));

        if (room->table.phase() == room::Phase::Playing) {
          TurnStart ts;
          ts.set_uid(room->table.playerId(room->table.currentTurn()));
          ts.set_deadline_ts(room->deadlineMs);
          ts.set_can_pass(room->table.canPass());
          s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_TurnStart), serialize(ts)));
        }
      }
    }
  }

  void Dispatcher::handlePlay(net::WsSession& s, const std::string& body) {
    PlayReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse PlayReq failed");
      return;
    }
    room::Room* room = rooms_.findMutable(s.roomId());
    if (!room || !room->started) {
      sendError(s, 404, "no active game");
      return;
    }
    const int seat = room::seatOf(*room, s.playerId());
    if (seat < 0) {
      sendError(s, 403, "not in room");
      return;
    }
    if (room->hostMode[seat]) {
      sendError(s, 409, "in host mode");
      return;
    }
    std::vector<guandan::rules::Card> cards;
    for (int i = 0; i < req.cards_size(); ++i) cards.push_back(req.cards(i));

    const room::PlayResult res = room->table.play(seat, cards);
    if (res != room::PlayResult::Ok) {
      sendError(s, errorCode(res), "play rejected");
      return;
    }

    PlayResultMsg prm;
    prm.set_uid(s.playerId());
    for (guandan::rules::Card c : cards) prm.add_cards(c);
    broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_PlayResult), serialize(prm));

    if (room->table.phase() == room::Phase::RoundEnd) {
      broadcastRoundEnd(*room);
      room->deadlineMs = 0;
      room->nextTickMs = 0;
    } else {
      beginTurn(*room);
    }
  }

  void Dispatcher::handlePass(net::WsSession& s, const std::string& body) {
    PassReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse PassReq failed");
      return;
    }
    room::Room* room = rooms_.findMutable(s.roomId());
    if (!room || !room->started) {
      sendError(s, 404, "no active game");
      return;
    }
    const int seat = room::seatOf(*room, s.playerId());
    if (seat < 0) {
      sendError(s, 403, "not in room");
      return;
    }
    if (room->hostMode[seat]) {
      sendError(s, 409, "in host mode");
      return;
    }
    const room::PlayResult res = room->table.pass(seat);
    if (res != room::PlayResult::Ok) {
      sendError(s, errorCode(res), "pass rejected");
      return;
    }

    PassResultMsg prm;
    prm.set_uid(s.playerId());
    broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_PassResult), serialize(prm));
    beginTurn(*room);
  }

  void Dispatcher::handleSetHost(net::WsSession& s, const std::string& body) {
    SetHostReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse SetHostReq failed");
      return;
    }
    room::Room* room = rooms_.findMutable(s.roomId());
    if (!room || !room->started) return;
    const int seat = room::seatOf(*room, s.playerId());
    if (seat < 0) return;
    room->hostMode[seat] = req.host();

    HostMode hm;
    hm.set_uid(s.playerId());
    hm.set_is_host(req.host());
    broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_HostMode), serialize(hm));
  }

  void Dispatcher::maybeStartGame(const std::string& roomId) {
    room::Room* room = rooms_.findMutable(roomId);
    if (!room || room->started) return;
    if (room->players.size() != static_cast<size_t>(room::RoomManager::kMaxPlayers)) return;

    std::array<int64_t, 4> ids{};
    for (int s = 0; s < 4; ++s) ids[s] = room->players[static_cast<size_t>(s)].playerId;
    room->table.startRound(ids, room->level, static_cast<uint32_t>(util::nowMs() & 0xffffffffu));
    room->started = true;

    GameStart gs;
    gs.set_level(room->level);
    gs.set_first_uid(room->table.playerId(0));
    broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_GameStart), serialize(gs));

    for (int s = 0; s < 4; ++s) {
      Deal deal;
      deal.set_level(room->level);
      deal.set_your_seat(s);
      for (guandan::rules::Card c : room->table.hand(s)) deal.add_cards(c);
      for (const auto& p : room->players) deal.add_seats(p.playerId);
      sendTo(room->table.playerId(s), static_cast<uint16_t>(Cmd::S2C_Deal), serialize(deal));
    }

    beginTurn(*room);
  }

  void Dispatcher::beginTurn(room::Room& room) {
    const int seat = room.table.currentTurn();
    if (seat < 0 || room.table.phase() != room::Phase::Playing) return;
    const int64_t now = util::nowMs();
    room.deadlineMs = now + kTurnTimeoutMs;
    room.nextTickMs = room.deadlineMs - kTickWindowMs;

    TurnStart ts;
    ts.set_uid(room.table.playerId(seat));
    ts.set_deadline_ts(room.deadlineMs);
    ts.set_can_pass(room.table.canPass());
    broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_TurnStart), serialize(ts));
  }

  void Dispatcher::broadcastAction(room::Room& room, int64_t uid,
                                   const std::vector<guandan::rules::Card>& cards, bool pass,
                                   int reasonTag) {
    AutoPlay ap;
    ap.set_uid(uid);
    ap.set_is_pass(pass);
    ap.set_reason_tag(reasonTag);
    if (pass) {
      broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_AutoPlay), serialize(ap));
      PassResultMsg prm;
      prm.set_uid(uid);
      broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_PassResult), serialize(prm));
    } else {
      for (guandan::rules::Card c : cards) ap.add_cards(c);
      broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_AutoPlay), serialize(ap));
      PlayResultMsg prm;
      prm.set_uid(uid);
      for (guandan::rules::Card c : cards) prm.add_cards(c);
      broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_PlayResult), serialize(prm));
    }
  }

  void Dispatcher::afterAction(room::Room& room) {
    if (room.table.phase() == room::Phase::RoundEnd) {
      broadcastRoundEnd(room);
      room.deadlineMs = 0;
      room.nextTickMs = 0;
    } else {
      beginTurn(room);
    }
  }

  void Dispatcher::autoResolve(room::Room& room, int seat, int64_t nowMs) {
    (void)nowMs;
    const int64_t uid = room.table.playerId(seat);
    if (room.table.canPass()) {
      room.table.pass(seat);
      broadcastAction(room, uid, {}, true);
    } else {
      const auto cards = room.table.minSingle(seat);
      room.table.play(seat, cards);
      broadcastAction(room, uid, cards, false);
    }
    afterAction(room);
  }

  void Dispatcher::handleStartMatch(net::WsSession& s, const std::string& body) {
    StartMatchReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse StartMatchReq failed");
      return;
    }
    if (!s.roomId().empty()) {
      sendError(s, 409, "already in room");
      return;
    }
    for (const auto& p : matchQueue_) {
      if (p.first == s.playerId()) return;  // 已在队列
    }
    matchQueue_.push_back({s.playerId(), util::nowMs()});
    GD_LOG_INFO("playerId={} start match, queue={}", s.playerId(), matchQueue_.size());
  }

  void Dispatcher::handleCancelMatch(net::WsSession& s, const std::string& body) {
    CancelMatchReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse CancelMatchReq failed");
      return;
    }
    matchQueue_.erase(
        std::remove_if(matchQueue_.begin(), matchQueue_.end(),
                       [&](const auto& p) { return p.first == s.playerId(); }),
        matchQueue_.end());
  }

  void Dispatcher::matchTick(int64_t nowMs) {
    constexpr int64_t kMatchTimeoutMs = 30000;
    std::vector<int64_t> players;
    if (matchQueue_.size() >= 4) {
      for (int i = 0; i < 4; ++i) players.push_back(matchQueue_[static_cast<size_t>(i)].first);
      matchQueue_.erase(matchQueue_.begin(), matchQueue_.begin() + 4);
    } else if (!matchQueue_.empty() && nowMs - matchQueue_.front().second > kMatchTimeoutMs) {
      for (const auto& p : matchQueue_) players.push_back(p.first);
      matchQueue_.clear();
    }
    if (players.empty()) return;

    const std::string roomId = rooms_.createRoom(players[0]);
    for (int64_t pid : players) {
      rooms_.joinRoom(roomId, pid);
      auto it = sessions_.find(pid);
      if (it != sessions_.end()) it->second->setRoomId(roomId);
    }
    while (true) {
      room::Room* room = rooms_.findMutable(roomId);
      if (!room || room->players.size() >= static_cast<size_t>(room::RoomManager::kMaxPlayers)) break;
      rooms_.addBot(roomId, nextBotId_);
      --nextBotId_;
    }

    for (int64_t pid : players) {
      MatchAck ack;
      ack.set_code(0);
      ack.set_room_id(roomId);
      ack.set_level(2);
      sendTo(pid, static_cast<uint16_t>(Cmd::S2C_MatchAck), serialize(ack));
    }

    maybeStartGame(roomId);
  }

  bool Dispatcher::isBotSeat(const room::Room& room, int seat) const {
    return seat >= 0 && seat < static_cast<int>(room.players.size()) && room.players[seat].isBot;
  }

  void Dispatcher::botResolve(room::Room& room, int seat) {
    const int64_t uid = room.table.playerId(seat);
    const auto& hand = room.table.hand(seat);
    const int level = room.table.level();
    const rules::PatternInfo* last = room.table.canPass() ? &room.table.lastPlay() : nullptr;

    ai::PlayContext ctx;
    ctx.canPass = room.table.canPass();
    ctx.lastPlayIsTeammate = room.table.lastPlaySeat() >= 0
                                 ? room::sameTeam(seat, room.table.lastPlaySeat())
                                 : false;
    ctx.minOpponentHand = 99;
    for (int s = 0; s < 4; ++s) {
      if (s == seat || room::sameTeam(s, seat)) continue;
      const int hc = room.table.handCount(s);
      if (hc < ctx.minOpponentHand) ctx.minOpponentHand = hc;
    }

    const ai::Decision d = ai::decide(hand, level, last, ctx, ai::Difficulty::Normal);
    if (d.pass) {
      if (room.table.canPass()) {
        room.table.pass(seat);
        broadcastAction(room, uid, {}, true, static_cast<int>(d.tag));
      } else {
        const auto safe = room.table.minSingle(seat);
        room.table.play(seat, safe);
        broadcastAction(room, uid, safe, false);
      }
    } else if (room.table.play(seat, d.cards) == room::PlayResult::Ok) {
      broadcastAction(room, uid, d.cards, false, static_cast<int>(d.tag));
    } else if (room.table.canPass()) {
      // 决策与规则引擎罕见不一致：兜底过牌
      room.table.pass(seat);
      broadcastAction(room, uid, {}, true);
    } else {
      // 领出兜底：最小单张
      const auto safe = room.table.minSingle(seat);
      room.table.play(seat, safe);
      broadcastAction(room, uid, safe, false);
    }
    afterAction(room);
  }

  void Dispatcher::handleAddBot(net::WsSession& s, const std::string& body) {
    AddBotReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse AddBotReq failed");
      return;
    }
    room::Room* room = rooms_.findMutable(s.roomId());
    if (!room || room->started) {
      sendError(s, 404, "room not found or already started");
      return;
    }
    int count = req.count() > 0 ? req.count() : 1;
    while (count-- > 0) {
      if (!rooms_.addBot(s.roomId(), nextBotId_)) break;
      --nextBotId_;
    }
    RoomState state;
    state.set_code(0);
    state.set_room_id(s.roomId());
    for (const auto& p : room->players) {
      auto* rp = state.add_players();
      rp->set_player_id(p.playerId);
      rp->set_name(p.name);
      rp->set_seat(p.seat);
      rp->set_is_bot(p.isBot);
    }
    broadcastRoom(*room, static_cast<uint16_t>(Cmd::S2C_RoomState), serialize(state));

    maybeStartGame(s.roomId());
  }

  void Dispatcher::handleHint(net::WsSession& s, const std::string& body) {
    HintReq req;
    if (!req.ParseFromString(body)) {
      sendError(s, 400, "parse HintReq failed");
      return;
    }
    room::Room* room = rooms_.findMutable(s.roomId());
    if (!room || !room->started) {
      sendError(s, 404, "no active game");
      return;
    }
    const int seat = room::seatOf(*room, s.playerId());
    if (seat < 0) {
      sendError(s, 403, "not in room");
      return;
    }
    if (room->table.currentTurn() != seat) {
      sendError(s, 409, "not your turn");
      return;
    }
    const int level = room->table.level();
    const rules::PatternInfo* last = room->table.canPass() ? &room->table.lastPlay() : nullptr;
    const auto entries = ai::hintTagged(room->table.hand(seat), level, last);

    HintAck ack;
    ack.set_code(0);
    for (const auto& e : entries) {
      auto* hp = ack.add_plays();
      for (guandan::rules::Card c : e.play.cards) hp->add_cards(c);
      hp->set_reason_tag(static_cast<int>(e.tag));
    }
    s.sendBinary(net::encodeFrame(static_cast<uint16_t>(Cmd::S2C_HintAck), serialize(ack)));
  }

  void Dispatcher::broadcastRoundEnd(room::Room& room) {
    RoundEnd re;
    for (const auto& f : room.table.finishOrder()) {
      re.add_finish_order(room.table.playerId(f.seat));
    }
    re.set_level_up(room.table.levelUp());
    re.set_last_uid(room.table.playerId(room.table.lastSeat()));
    broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_RoundEnd), serialize(re));

    // 段位结算
    if (ratingsOpen_) {
      const int headSeat = room.table.headSeat();
      std::pair<int64_t, int> a1, a2, b1, b2;
      int ai = 0, bi = 0;
      for (int s = 0; s < 4; ++s) {
        const int64_t pid = room.table.playerId(s);
        const int r = ratings_.getRating(pid);
        if ((s & 1) == 0) {
          if (ai++ == 0) a1 = {pid, r}; else a2 = {pid, r};
        } else {
          if (bi++ == 0) b1 = {pid, r}; else b2 = {pid, r};
        }
      }
      const bool evenTeamWins = (headSeat & 1) == 0;
      const auto ups = rank::computeRatings(a1, a2, b1, b2, evenTeamWins);

      RankUpdate ru;
      for (const auto& u : ups) {
        ratings_.setRating(u.playerId, u.newRating);
        auto* info = ru.add_updates();
        info->set_player_id(u.playerId);
        info->set_old_rating(u.oldRating);
        info->set_new_rating(u.newRating);
        info->set_delta(u.delta);
        info->set_tier(rank::tierName(u.newRating));
      }
      broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_RankUpdate), serialize(ru));
    }
  }

  int Dispatcher::errorCode(room::PlayResult r) const {
    switch (r) {
      case room::PlayResult::NotYourTurn: return 403;
      case room::PlayResult::InvalidCards:
      case room::PlayResult::CardsNotInHand: return 400;
      case room::PlayResult::CannotBeat:
      case room::PlayResult::MustLead: return 409;
      default: return 400;
    }
  }

  void Dispatcher::onTick(int64_t nowMs) {
    matchTick(nowMs);
    rooms_.forEachRoom([&](room::Room& room) {
      if (!room.started || room.table.phase() != room::Phase::Playing) return;

      // 连续自动（Bot/托管/超时）在同一次 tick 内尽量一次清完，避免每次 poll 只动一步。
      for (int guard = 0; guard < 128; ++guard) {
        if (room.table.phase() != room::Phase::Playing) break;
        const int seat = room.table.currentTurn();
        if (seat < 0) break;

        if (isBotSeat(room, seat)) {
          botResolve(room, seat);
        } else if (room.hostMode[seat]) {
          autoResolve(room, seat, nowMs);
        } else if (room.deadlineMs > 0 && nowMs >= room.deadlineMs) {
          ++room.timeoutCount[seat];
          autoResolve(room, seat, nowMs);
        } else {
          break;  // 需要真人操作
        }
      }

      // 最后 5s 推 Tick
      if (room.table.phase() == room::Phase::Playing) {
        const int seat = room.table.currentTurn();
        if (seat >= 0 && room.nextTickMs > 0 && nowMs >= room.nextTickMs &&
            room.deadlineMs > nowMs) {
          const int remain = static_cast<int>(room.deadlineMs - nowMs);
          Tick tick;
          tick.set_remain_ms(remain);
          broadcastRoom(room, static_cast<uint16_t>(Cmd::S2C_Tick), serialize(tick));
          room.nextTickMs = nowMs + 1000;
        }
      }
    });
  }

}  // namespace guandan::proto



