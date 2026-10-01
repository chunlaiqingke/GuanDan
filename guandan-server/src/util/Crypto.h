#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace guandan::util {

  // RFC 6455 握手 GUID
  inline constexpr char kWebSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  // SHA-1，输出 20 字节。
  std::array<uint8_t, 20> sha1(const uint8_t* data, size_t len);

  // SHA-1 十六进制（小写），便于测试对比。
  std::string sha1Hex(const std::string& input);

  // 标准 Base64 编码。
  std::string base64Encode(const uint8_t* data, size_t len);

  // 计算 Sec-WebSocket-Accept：base64(sha1(key + GUID))。
  std::string wsAcceptKey(const std::string& secWebSocketKey);

}  // namespace guandan::util
