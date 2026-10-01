#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "messages.pb.h"
#include "minitest.h"
#include "net/Frame.h"
#include "net/WsServer.h"
#include "proto/Dispatcher.h"
#include "room/RoomManager.h"

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
    frame.push_back(0x82);  // FIN + binary
    const uint8_t maskKey[4] = {0x11, 0x22, 0x33, 0x44};
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
    frame.insert(frame.end(), maskKey, maskKey + 4);
    for (size_t i = 0; i < len; ++i) frame.push_back(payload[i] ^ maskKey[i & 3]);
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

}  // namespace

TEST(WsIntegrationLoginHeartbeat) {
  guandan::room::RoomManager rooms;
  guandan::proto::Dispatcher dispatcher(rooms);
  guandan::net::WsServer server;
  server.setOnBinary([&](guandan::net::WsSession& s, const uint8_t* d, size_t n) {
    dispatcher.onBinary(s, d, n);
  });

  if (!server.listen(0)) {
    ::minitest::reportFailure("listen(0) failed", __FILE__, __LINE__);
    return;
  }
  const uint16_t port = server.port();
  std::thread t([&]() { server.run(); });

  try {
    const int fd = connectTcp(port);

    // 握手
    const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    const std::string req = "GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\n"
                            "Connection: Upgrade\r\nSec-WebSocket-Key: " +
                            key + "\r\nSec-WebSocket-Version: 13\r\n\r\n";
    writeAll(fd, std::vector<uint8_t>(req.begin(), req.end()));
    const std::string resp = readHttpResponse(fd);
    EXPECT_TRUE(resp.find("101") != std::string::npos);
    EXPECT_TRUE(resp.find("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != std::string::npos);

    // 登录
    guandan::LoginReq login;
    login.set_uid("alice");
    login.set_token("tok");
    std::string body;
    login.SerializeToString(&body);
    sendMaskedFrame(fd, guandan::net::encodeFrame(1001, body));

    const auto respFrame = recvFrame(fd);
    guandan::net::FrameHeader hdr;
    EXPECT_TRUE(guandan::net::decodeHeader(respFrame.data(), respFrame.size(), hdr));
    EXPECT_EQ(hdr.cmd, 1002);
    guandan::LoginAck ack;
    EXPECT_TRUE(ack.ParseFromString(std::string(respFrame.begin() + 8, respFrame.end())));
    EXPECT_EQ(ack.code(), 0);
    EXPECT_TRUE(ack.player_id() > 0);

    // 心跳
    guandan::HeartbeatReq hb;
    hb.set_ts(12345);
    std::string hbBody;
    hb.SerializeToString(&hbBody);
    sendMaskedFrame(fd, guandan::net::encodeFrame(9001, hbBody));

    const auto hbResp = recvFrame(fd);
    guandan::net::FrameHeader hbHdr;
    EXPECT_TRUE(guandan::net::decodeHeader(hbResp.data(), hbResp.size(), hbHdr));
    EXPECT_EQ(hbHdr.cmd, 9002);
    guandan::HeartbeatAck hbAck;
    EXPECT_TRUE(hbAck.ParseFromString(std::string(hbResp.begin() + 8, hbResp.end())));
    EXPECT_EQ(hbAck.ts(), 12345);

    ::close(fd);
  } catch (const std::exception& e) {
    ::minitest::reportFailure(std::string("exception: ") + e.what(), __FILE__, __LINE__);
  }

  server.stop();
  t.join();
}

MINITEST_MAIN()
