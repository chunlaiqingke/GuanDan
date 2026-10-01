#include "net/Frame.h"

#include <cstring>

namespace guandan::net {

  bool decodeHeader(const uint8_t* data, size_t size, FrameHeader& out) {
    if (size < kFrameHeaderSize) return false;
    out.cmd = readU16(data);
    out.ver = readU16(data + 2);
    out.len = readU32(data + 4);
    return true;
  }

  std::vector<uint8_t> encodeFrame(uint16_t cmd, const std::string& body) {
    std::vector<uint8_t> frame;
    frame.resize(kFrameHeaderSize + body.size());
    uint8_t* p = frame.data();
    writeU16(p, cmd);
    writeU16(p + 2, kVersion);
    writeU32(p + 4, static_cast<uint32_t>(body.size()));
    if (!body.empty()) {
      std::memcpy(p + kFrameHeaderSize, body.data(), body.size());
    }
    return frame;
  }

}  // namespace guandan::net
