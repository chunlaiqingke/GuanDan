#include <cstdint>
#include <cstdlib>

#include "net/WsServer.h"
#include "proto/Dispatcher.h"
#include "room/RoomManager.h"
#include "util/Log.h"
#include "util/Random.h"
#include "util/Time.h"

int main(int argc, char** argv) {
  guandan::util::initLog();
  guandan::util::seedRand(static_cast<uint32_t>(guandan::util::nowMs() & 0xffffffffu));

  uint16_t port = 9001;
  if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));

  guandan::room::RoomManager rooms;
  guandan::proto::Dispatcher dispatcher(rooms);
  dispatcher.openRatings(argc > 2 ? argv[2] : "guandan.db");
  guandan::net::WsServer server;
  server.setOnBinary(
      [&](guandan::net::WsSession& s, const uint8_t* d, size_t n) { dispatcher.onBinary(s, d, n); });
  server.setOnDisconnect(
      [&](guandan::net::WsSession& s) { dispatcher.onDisconnect(s); });
  server.setOnTick(
      [&](int64_t nowMs) { dispatcher.onTick(nowMs); });

  if (!server.listen(port)) {
    GD_LOG_ERROR("listen on port {} failed", port);
    return 1;
  }
  GD_LOG_INFO("guandan server listening on ws://0.0.0.0:{}", server.port());
  server.run();
  return 0;
}
