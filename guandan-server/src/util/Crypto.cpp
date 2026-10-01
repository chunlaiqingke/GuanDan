#include "util/Crypto.h"

#include <vector>

namespace guandan::util {
namespace {

inline uint32_t rotl(uint32_t x, int n) {
  return (x << n) | (x >> (32 - n));
}

}  // namespace

std::array<uint8_t, 20> sha1(const uint8_t* data, size_t len) {
  const uint64_t bitLen = static_cast<uint64_t>(len) * 8;

  // 填充：0x80 + 若干 0x00 直到 (size % 64) == 56，再追加 8 字节大端长度。
  std::vector<uint8_t> msg(data, data + len);
  msg.push_back(0x80);
  while ((msg.size() % 64) != 56) msg.push_back(0x00);
  for (int i = 7; i >= 0; --i) {
    msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xff));
  }

  uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};

  for (size_t off = 0; off < msg.size(); off += 64) {
    uint32_t w[80];
    for (int i = 0; i < 16; ++i) {
      const uint8_t* p = &msg[off + static_cast<size_t>(i) * 4];
      w[i] = (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
             (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
    }
    for (int i = 16; i < 80; ++i) {
      w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; ++i) {
      uint32_t f, k;
      if (i < 20) {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999u;
      } else if (i < 40) {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1u;
      } else if (i < 60) {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDCu;
      } else {
        f = b ^ c ^ d;
        k = 0xCA62C1D6u;
      }
      const uint32_t tmp = rotl(a, 5) + f + e + k + w[i];
      e = d;
      d = c;
      c = rotl(b, 30);
      b = a;
      a = tmp;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
  }

  std::array<uint8_t, 20> out{};
  for (int i = 0; i < 5; ++i) {
    out[static_cast<size_t>(i) * 4 + 0] = static_cast<uint8_t>((h[i] >> 24) & 0xff);
    out[static_cast<size_t>(i) * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xff);
    out[static_cast<size_t>(i) * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xff);
    out[static_cast<size_t>(i) * 4 + 3] = static_cast<uint8_t>(h[i] & 0xff);
  }
  return out;
}

std::string sha1Hex(const std::string& input) {
  const auto d = sha1(reinterpret_cast<const uint8_t*>(input.data()), input.size());
  static const char* hex = "0123456789abcdef";
  std::string out;
  out.reserve(40);
  for (const uint8_t b : d) {
    out.push_back(hex[b >> 4]);
    out.push_back(hex[b & 0xf]);
  }
  return out;
}

std::string base64Encode(const uint8_t* data, size_t len) {
  static const char* tbl =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((len + 2) / 3) * 4);
  size_t i = 0;
  while (i + 3 <= len) {
    const uint32_t v = (static_cast<uint32_t>(data[i]) << 16) |
                       (static_cast<uint32_t>(data[i + 1]) << 8) |
                       static_cast<uint32_t>(data[i + 2]);
    out.push_back(tbl[(v >> 18) & 0x3f]);
    out.push_back(tbl[(v >> 12) & 0x3f]);
    out.push_back(tbl[(v >> 6) & 0x3f]);
    out.push_back(tbl[v & 0x3f]);
    i += 3;
  }
  if (i + 1 == len) {
    const uint32_t v = static_cast<uint32_t>(data[i]) << 16;
    out.push_back(tbl[(v >> 18) & 0x3f]);
    out.push_back(tbl[(v >> 12) & 0x3f]);
    out.push_back('=');
    out.push_back('=');
  } else if (i + 2 == len) {
    const uint32_t v = (static_cast<uint32_t>(data[i]) << 16) |
                       (static_cast<uint32_t>(data[i + 1]) << 8);
    out.push_back(tbl[(v >> 18) & 0x3f]);
    out.push_back(tbl[(v >> 12) & 0x3f]);
    out.push_back(tbl[(v >> 6) & 0x3f]);
    out.push_back('=');
  }
  return out;
}

std::string wsAcceptKey(const std::string& key) {
  const std::string input = key + kWebSocketGuid;
  const auto d = sha1(reinterpret_cast<const uint8_t*>(input.data()), input.size());
  return base64Encode(d.data(), d.size());
}

}  // namespace guandan::util
