#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace guandan::net {

// 业务帧格式（小端）：[2B cmd][2B ver][4B len][protobuf body]
// 每个 WS 二进制帧 = 一条完整业务消息，不跨帧拼包。
constexpr uint16_t kVersion = 1;
constexpr size_t kFrameHeaderSize = 8;  // 2 + 2 + 4

struct FrameHeader {
  uint16_t cmd = 0;
  uint16_t ver = 0;
  uint32_t len = 0;
};

// ---- 小端读写 ----
inline uint16_t readU16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

inline uint32_t readU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

inline void writeU16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v & 0xff);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xff);
}

inline void writeU32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v & 0xff);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xff);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xff);
  p[3] = static_cast<uint8_t>((v >> 24) & 0xff);
}

// 解析帧头；不足 8 字节返回 false。
bool decodeHeader(const uint8_t* data, size_t size, FrameHeader& out);

// 组装完整业务帧：cmd + body -> 帧字节。
std::vector<uint8_t> encodeFrame(uint16_t cmd, const std::string& body);

}  // namespace guandan::net
