#include <string>

#include "minitest.h"
#include "util/Crypto.h"

using namespace guandan::util;

TEST(Sha1KnownVectors) {
  EXPECT_STREQ(sha1Hex(""), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
  EXPECT_STREQ(sha1Hex("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
  EXPECT_STREQ(sha1Hex("The quick brown fox jumps over the lazy dog"),
               "2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");
}

TEST(Base64KnownVectors) {
  EXPECT_STREQ(base64Encode(reinterpret_cast<const uint8_t*>(""), 0), "");
  EXPECT_STREQ(base64Encode(reinterpret_cast<const uint8_t*>("f"), 1), "Zg==");
  EXPECT_STREQ(base64Encode(reinterpret_cast<const uint8_t*>("fo"), 2), "Zm8=");
  EXPECT_STREQ(base64Encode(reinterpret_cast<const uint8_t*>("foo"), 3), "Zm9v");
  EXPECT_STREQ(base64Encode(reinterpret_cast<const uint8_t*>("foob"), 4), "Zm9vYg==");
}

TEST(WsAcceptKeyRfcExample) {
  // RFC 6455 第 1.3 节示例
  EXPECT_STREQ(wsAcceptKey("dGhlIHNhbXBsZSBub25jZQ=="), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

MINITEST_MAIN()
