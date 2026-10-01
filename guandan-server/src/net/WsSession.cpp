#include "net/WsSession.h"

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sstream>

#include "util/Crypto.h"

namespace guandan::net {

  namespace {
    // 单帧 payload 上限，防御异常大帧。
    constexpr size_t kMaxFramePayload = 1 << 20;  // 1MB
    constexpr size_t kMaxInputBuffer = 1 << 20;

    std::string toLower(std::string s) {
      std::transform(s.begin(), s.end(), s.begin(),
                     [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
      return s;
    }

    std::string trim(const std::string& s) {
      size_t b = 0;
      size_t e = s.size();
      while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
      while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
      return s.substr(b, e - b);
    }
  }  // namespace

  WsSession::WsSession(int fd) : fd_(fd) {}

  WsSession::~WsSession() {
    if (fd_ >= 0) ::close(fd_);
  }

  void WsSession::appendInput(const uint8_t* data, size_t len) {
    inBuf_.insert(inBuf_.end(), data, data + len);
    if (inBuf_.size() > kMaxInputBuffer) {
      close();
    }
  }

  bool WsSession::tryHandshake() {
    const std::string marker = "\r\n\r\n";
    const auto it = std::search(inBuf_.begin(), inBuf_.end(), marker.begin(), marker.end());
    if (it == inBuf_.end()) return true;  // 数据不足，继续等

    const size_t headerLen = static_cast<size_t>(it - inBuf_.begin()) + marker.size();
    std::string req(inBuf_.begin(), inBuf_.begin() + static_cast<long>(headerLen));

    if (req.rfind("GET ", 0) != 0) return false;

    std::string key;
    bool upgrade = false;
    std::istringstream iss(req);
    std::string line;
    while (std::getline(iss, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      const size_t colon = line.find(':');
      if (colon == std::string::npos) continue;
      const std::string name = toLower(trim(line.substr(0, colon)));
      const std::string value = trim(line.substr(colon + 1));
      if (name == "sec-websocket-key") {
        key = value;
      } else if (name == "upgrade") {
        upgrade = (toLower(value) == "websocket");
      }
    }
    if (key.empty() || !upgrade) return false;

    const std::string accept = util::wsAcceptKey(key);
    std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
                       "Upgrade: websocket\r\n"
                       "Connection: Upgrade\r\n"
                       "Sec-WebSocket-Accept: " +
                       accept + "\r\n\r\n";
    writeRaw(reinterpret_cast<const uint8_t*>(resp.data()), resp.size());

    inBuf_.erase(inBuf_.begin(), inBuf_.begin() + static_cast<long>(headerLen));
    state_ = SessionState::Open;
    return true;
  }

  bool WsSession::process(const BinaryCallback& onBinary) {
    if (state_ == SessionState::Handshake) {
      if (!tryHandshake()) return false;
      if (state_ != SessionState::Open) return true;  // 数据不足
    }

    while (state_ == SessionState::Open) {
      const size_t n = inBuf_.size();
      if (n < 2) break;

      const uint8_t b0 = inBuf_[0];
      const uint8_t b1 = inBuf_[1];
      const bool fin = (b0 & 0x80) != 0;
      const uint8_t opcode = b0 & 0x0f;
      const bool masked = (b1 & 0x80) != 0;
      uint64_t len = b1 & 0x7f;
      size_t idx = 2;

      if (len == 126) {
        if (n < 4) break;
        len = (static_cast<uint64_t>(inBuf_[2]) << 8) | inBuf_[3];
        idx = 4;
      } else if (len == 127) {
        if (n < 10) break;
        len = 0;
        for (int i = 0; i < 8; ++i) len = (len << 8) | inBuf_[2 + static_cast<size_t>(i)];
        idx = 10;
      }

      uint8_t maskKey[4] = {0, 0, 0, 0};
      if (masked) {
        if (n < idx + 4) break;
        maskKey[0] = inBuf_[idx];
        maskKey[1] = inBuf_[idx + 1];
        maskKey[2] = inBuf_[idx + 2];
        maskKey[3] = inBuf_[idx + 3];
        idx += 4;
      }

      if (len > kMaxFramePayload) return false;
      if (n < idx + len) break;  // 帧不完整

      if (!masked) return false;  // 客户端帧必须掩码

      for (uint64_t i = 0; i < len; ++i) {
        inBuf_[idx + i] ^= maskKey[i & 3];
      }

      const uint8_t* payload = inBuf_.data() + idx;

      switch (opcode) {
        case 0x2:
          if (!fin) return false;  // 不支持分片
          if (onBinary) onBinary(*this, payload, static_cast<size_t>(len));
          break;
        case 0x9:
          if (fin) {
            std::vector<uint8_t> pong(2 + len);
            pong[0] = 0x8A;  // FIN + pong
            pong[1] = static_cast<uint8_t>(len);
            std::copy(payload, payload + len, pong.begin() + 2);
            writeRaw(pong.data(), pong.size());
          }
          break;
        case 0xA:
          break;
        case 0x8:
          if (fin) {
            close();
            return false;
          }
          break;
        case 0x1:
          break;  // 项目只用二进制，忽略文本帧
        default:
          return false;
      }

      inBuf_.erase(inBuf_.begin(), inBuf_.begin() + static_cast<long>(idx + len));
    }
    return true;
  }

  void WsSession::sendBinary(const std::vector<uint8_t>& payload) {
    if (state_ != SessionState::Open) return;
    std::vector<uint8_t> frame;
    const size_t len = payload.size();
    frame.reserve(2 + 10 + len);
    frame.push_back(0x82);  // FIN + binary
    if (len < 126) {
      frame.push_back(static_cast<uint8_t>(len));
    } else if (len <= 0xFFFF) {
      frame.push_back(126);
      frame.push_back(static_cast<uint8_t>((len >> 8) & 0xff));
      frame.push_back(static_cast<uint8_t>(len & 0xff));
    } else {
      frame.push_back(127);
      for (int i = 7; i >= 0; --i) {
        frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xff));
      }
    }
    frame.insert(frame.end(), payload.begin(), payload.end());
    writeRaw(frame.data(), frame.size());
  }

  void WsSession::flush() {
    while (outOff_ < outBuf_.size()) {
      const ssize_t n = ::send(fd_, outBuf_.data() + outOff_, outBuf_.size() - outOff_, 0);
      if (n > 0) {
        outOff_ += static_cast<size_t>(n);
        continue;
      }
      if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return;
      outBuf_.clear();
      outOff_ = 0;
      state_ = SessionState::Closing;
      return;
    }
    outBuf_.clear();
    outOff_ = 0;
  }

  void WsSession::close() {
    if (state_ == SessionState::Closing) return;
    const uint8_t closeFrame[2] = {0x88, 0x00};  // FIN + close, 空 payload
    writeRaw(closeFrame, sizeof(closeFrame));
    state_ = SessionState::Closing;
  }

}  // namespace guandan::net
