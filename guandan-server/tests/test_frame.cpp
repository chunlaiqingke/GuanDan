#include <cstdint>
#include <string>
#include <vector>

#include "minitest.h"
#include "net/Frame.h"

using namespace guandan::net;

TEST(FrameEncodeDecodeRoundTrip) {
  const std::string body = "hello";
  const auto frame = encodeFrame(1001, body);
  EXPECT_EQ(frame.size(), kFrameHeaderSize + body.size());
  EXPECT_EQ(readU16(frame.data()), 1001);
  EXPECT_EQ(readU16(frame.data() + 2), kVersion);
  EXPECT_EQ(readU32(frame.data() + 4), static_cast<uint32_t>(body.size()));

  FrameHeader hdr;
  EXPECT_TRUE(decodeHeader(frame.data(), frame.size(), hdr));
  EXPECT_EQ(hdr.cmd, 1001);
  EXPECT_EQ(hdr.ver, kVersion);
  EXPECT_EQ(hdr.len, static_cast<uint32_t>(body.size()));
  EXPECT_STREQ(std::string(frame.begin() + kFrameHeaderSize, frame.end()), body);
}

TEST(FrameLittleEndian) {
  uint8_t buf[4] = {0, 0, 0, 0};
  writeU16(buf, 0x1234);
  EXPECT_EQ(buf[0], 0x34);
  EXPECT_EQ(buf[1], 0x12);
  EXPECT_EQ(readU16(buf), 0x1234);

  writeU32(buf, 0x12345678);
  EXPECT_EQ(buf[0], 0x78);
  EXPECT_EQ(buf[1], 0x56);
  EXPECT_EQ(buf[2], 0x34);
  EXPECT_EQ(buf[3], 0x12);
  EXPECT_EQ(readU32(buf), 0x12345678u);
}

TEST(FrameDecodeHeaderTooShort) {
  uint8_t shortBuf[4] = {0, 0, 0, 0};
  FrameHeader hdr;
  EXPECT_FALSE(decodeHeader(shortBuf, 4, hdr));
}

TEST(FrameEmptyBody) {
  const auto frame = encodeFrame(9001, "");
  EXPECT_EQ(frame.size(), kFrameHeaderSize);
  FrameHeader hdr;
  EXPECT_TRUE(decodeHeader(frame.data(), frame.size(), hdr));
  EXPECT_EQ(hdr.cmd, 9001);
  EXPECT_EQ(hdr.len, 0u);
}

MINITEST_MAIN()
