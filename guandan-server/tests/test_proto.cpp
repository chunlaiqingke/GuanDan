#include <string>

#include "messages.pb.h"
#include "minitest.h"
#include "room/RoomManager.h"

TEST(ProtoLoginReqRoundTrip) {
  guandan::LoginReq req;
  req.set_uid("alice");
  req.set_token("tok123");
  std::string body;
  EXPECT_TRUE(req.SerializeToString(&body));

  guandan::LoginReq req2;
  EXPECT_TRUE(req2.ParseFromString(body));
  EXPECT_STREQ(req2.uid(), "alice");
  EXPECT_STREQ(req2.token(), "tok123");
}

TEST(ProtoLoginAckRoundTrip) {
  guandan::LoginAck ack;
  ack.set_code(0);
  ack.set_player_id(42);
  ack.set_msg("ok");
  std::string body;
  EXPECT_TRUE(ack.SerializeToString(&body));

  guandan::LoginAck ack2;
  EXPECT_TRUE(ack2.ParseFromString(body));
  EXPECT_EQ(ack2.code(), 0);
  EXPECT_EQ(ack2.player_id(), 42);
  EXPECT_STREQ(ack2.msg(), "ok");
}

TEST(ProtoRoomStateRoundTrip) {
  guandan::RoomState state;
  state.set_code(0);
  state.set_room_id("123456");
  auto* p = state.add_players();
  p->set_player_id(1);
  p->set_seat(0);
  p->set_name("alice");
  std::string body;
  EXPECT_TRUE(state.SerializeToString(&body));

  guandan::RoomState state2;
  EXPECT_TRUE(state2.ParseFromString(body));
  EXPECT_STREQ(state2.room_id(), "123456");
  EXPECT_EQ(state2.players_size(), 1);
  EXPECT_EQ(state2.players(0).player_id(), 1);
  EXPECT_STREQ(state2.players(0).name(), "alice");
}

TEST(RoomManagerCreateJoin) {
  guandan::room::RoomManager mgr;
  const std::string id = mgr.createRoom(1);
  EXPECT_TRUE(mgr.exists(id));
  EXPECT_FALSE(mgr.exists("nope"));

  EXPECT_TRUE(mgr.joinRoom(id, 2));
  EXPECT_TRUE(mgr.joinRoom(id, 3));
  EXPECT_TRUE(mgr.joinRoom(id, 4));
  EXPECT_TRUE(mgr.joinRoom(id, 5));   // 第 4 人
  EXPECT_FALSE(mgr.joinRoom(id, 6));  // 已满
  EXPECT_TRUE(mgr.joinRoom(id, 2));   // 重复加入返回 true

  const auto* room = mgr.find(id);
  EXPECT_TRUE(room != nullptr);
  EXPECT_EQ(room->players.size(), 4u);
  EXPECT_EQ(room->players[0].seat, 0);
  EXPECT_EQ(room->players[3].seat, 3);
}

MINITEST_MAIN()
