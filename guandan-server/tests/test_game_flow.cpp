#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "messages.pb.h"
#include "minitest.h"
#include "net/Frame.h"
#include "net/WsServer.h"
#include "proto/Dispatcher.h"
#include "room/RoomManager.h"

using namespace guandan;

namespace {

  void writeAll(int fd, const std::vector<uint8_t>& data) {
    size_t off = 0;
    while (off < data.size()) {
      const ssize_t w = ::send(fd, data.data() + off, data.size() - off, 0);
      if (w <= 0) throw std::runtime_error("send failed");
      off += static_cast<size_t>(w);
    }
  }

  void readAll(int fd, uint8_t* buf, size_t n) {
    size_t off = 0;
    while (off < n) {
      const ssize_t r = ::recv(fd, buf + off, n - off, 0);
      if (r <= 0) throw std::runtime_error("recv failed");
      off += static_cast<size_t>(r);
    }
  }

  int connectTcp(uint16_t port) {
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) throw std::runtime_error("socket failed");
#ifdef SO_NOSIGPIPE
    int one = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one));
#endif
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
      throw std::runtime_error("connect failed");
    }
    return fd;
  }

  std::string readHttpResponse(int fd) {
    std::string s;
    char c = 0;
    while (s.size() < 4096) {
      const ssize_t r = ::recv(fd, &c, 1, 0);
      if (r <= 0) throw std::runtime_error("recv failed");
      s.push_back(c);
      if (s.size() >= 4 && s.substr(s.size() - 4) == "\r\n\r\n") break;
    }
    return s;
  }

  void sendMaskedFrame(int fd, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> frame;
    frame.push_back(0x82);
    const uint8_t mk[4] = {0x11, 0x22, 0x33, 0x44};
    const size_t len = payload.size();
    if (len < 126) {
      frame.push_back(0x80 | static_cast<uint8_t>(len));
    } else if (len <= 0xFFFF) {
      frame.push_back(0x80 | 126);
      frame.push_back(static_cast<uint8_t>((len >> 8) & 0xff));
      frame.push_back(static_cast<uint8_t>(len & 0xff));
    } else {
      frame.push_back(0x80 | 127);
      for (int i = 7; i >= 0; --i) frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xff));
    }
    frame.insert(frame.end(), mk, mk + 4);
    for (size_t i = 0; i < len; ++i) frame.push_back(payload[i] ^ mk[i & 3]);
    writeAll(fd, frame);
  }

  std::vector<uint8_t> recvFrame(int fd) {
    uint8_t hdr[2] = {0, 0};
    readAll(fd, hdr, 2);
    const bool masked = (hdr[1] & 0x80) != 0;
    uint64_t len = hdr[1] & 0x7f;
    if (len == 126) {
      uint8_t ext[2] = {0, 0};
      readAll(fd, ext, 2);
      len = (static_cast<uint64_t>(ext[0]) << 8) | ext[1];
    } else if (len == 127) {
      uint8_t ext[8] = {0};
      readAll(fd, ext, 8);
      len = 0;
      for (int i = 0; i < 8; ++i) len = (len << 8) | ext[i];
    }
    uint8_t mk[4] = {0, 0, 0, 0};
    if (masked) readAll(fd, mk, 4);
    std::vector<uint8_t> payload(static_cast<size_t>(len));
    if (len) readAll(fd, payload.data(), static_cast<size_t>(len));
    if (masked) {
      for (size_t i = 0; i < len; ++i) payload[i] ^= mk[i & 3];
    }
    return payload;
  }

  bool recvFrameTimeout(int fd, int ms, std::vector<uint8_t>& out) {
    pollfd p{};
    p.fd = fd;
    p.events = POLLIN;
    const int r = ::poll(&p, 1, ms);
    if (r <= 0) return false;
    if (!(p.revents & POLLIN)) return false;
    out = recvFrame(fd);
    return true;
  }

  void doHandshake(int fd) {
    const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    const std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                            "Connection: Upgrade\r\nSec-WebSocket-Key: " +
                            key + "\r\nSec-WebSocket-Version: 13\r\n\r\n";
    writeAll(fd, std::vector<uint8_t>(req.begin(), req.end()));
    const std::string resp = readHttpResponse(fd);
    if (resp.find("101") == std::string::npos) throw std::runtime_error("handshake failed");
  }

  int minSingleCard(const std::vector<int32_t>& hand) {
    int32_t best = hand[0];
    for (int32_t c : hand) {
      if ((c >> 2) < (best >> 2)) best = c;
    }
    return best;
  }

  template <typename M>
  std::string serialize(const M& m) {
    std::string out;
    m.SerializeToString(&out);
    return out;
  }

}  // namespace

TEST(GameFlowFullRound) {
  room::RoomManager rooms;
  proto::Dispatcher dispatcher(rooms);
  net::WsServer server;
  server.setOnBinary([&](net::WsSession& s, const uint8_t* d, size_t n) { dispatcher.onBinary(s, d, n); });
  server.setOnDisconnect([&](net::WsSession& s) { dispatcher.onDisconnect(s); });
  server.setOnTick([&](int64_t t) { dispatcher.onTick(t); });

  if (!server.listen(0)) {
    ::minitest::reportFailure("listen(0) failed", __FILE__, __LINE__);
    return;
  }
  const uint16_t port = server.port();
  std::thread th([&]() { server.run(); });

  try {
    int fd[4] = {-1, -1, -1, -1};
    int64_t pid[4] = {0, 0, 0, 0};
    std::unordered_map<int64_t, int> fdOf;

    for (int i = 0; i < 4; ++i) {
      fd[i] = connectTcp(port);
      doHandshake(fd[i]);
      LoginReq login;
      login.set_uid("p" + std::to_string(i));
      login.set_token("t");
      sendMaskedFrame(fd[i], net::encodeFrame(1001, serialize(login)));
      const auto f = recvFrame(fd[i]);
      net::FrameHeader hdr;
      net::decodeHeader(f.data(), f.size(), hdr);
      LoginAck ack;
      ack.ParseFromString(std::string(f.begin() + 8, f.end()));
      pid[i] = ack.player_id();
      fdOf[pid[i]] = i;
    }

    std::string roomId;
    {
      CreateRoomReq cr;
      cr.mutable_rule()->set_level(2);
      sendMaskedFrame(fd[0], net::encodeFrame(2001, serialize(cr)));
      const auto f = recvFrame(fd[0]);
      net::FrameHeader hdr;
      net::decodeHeader(f.data(), f.size(), hdr);
      CreateRoomAck ack;
      ack.ParseFromString(std::string(f.begin() + 8, f.end()));
      roomId = ack.room_id();
    }
    for (int i = 1; i < 4; ++i) {
      JoinRoomReq jr;
      jr.set_room_id(roomId);
      sendMaskedFrame(fd[i], net::encodeFrame(2003, serialize(jr)));
    }

    // 驱动一局：轮到自己时领出最小单张、跟牌则过。
    std::unordered_map<int64_t, std::vector<int32_t>> hand;
    int64_t currentUid = 0;
    bool canPass = false;
    bool pending = false;
    bool roundEnded = false;

    for (int step = 0; step < 2000 && !roundEnded; ++step) {
      for (int i = 0; i < 4; ++i) {
        std::vector<uint8_t> f;
        while (recvFrameTimeout(fd[i], 2, f)) {
          net::FrameHeader hdr;
          if (!net::decodeHeader(f.data(), f.size(), hdr)) continue;
          const std::string body(f.begin() + 8, f.end());
          if (hdr.cmd == 3008) {
            Deal d;
            d.ParseFromString(body);
            hand[pid[i]].assign(d.cards().begin(), d.cards().end());
          } else if (hdr.cmd == 3001) {
            TurnStart ts;
            ts.ParseFromString(body);
            currentUid = ts.uid();
            canPass = ts.can_pass();
            pending = true;
          } else if (hdr.cmd == 3009) {
            roundEnded = true;
          }
        }
      }

      if (pending && currentUid != 0) {
        pending = false;
        const int fi = fdOf[currentUid];
        if (canPass) {
          PassReq p;
          sendMaskedFrame(fd[fi], net::encodeFrame(3007, serialize(p)));
        } else {
          auto& h = hand[currentUid];
          const int32_t c = minSingleCard(h);
          PlayReq pr;
          pr.add_cards(c);
          sendMaskedFrame(fd[fi], net::encodeFrame(3006, serialize(pr)));
          for (auto it = h.begin(); it != h.end(); ++it) {
            if (*it == c) {
              h.erase(it);
              break;
            }
          }
        }
      }
    }

    EXPECT_TRUE(roundEnded);

    for (int i = 0; i < 4; ++i) ::close(fd[i]);
  } catch (const std::exception& e) {
    ::minitest::reportFailure(std::string("exception: ") + e.what(), __FILE__, __LINE__);
  }

  server.stop();
  th.join();
}

MINITEST_MAIN()


