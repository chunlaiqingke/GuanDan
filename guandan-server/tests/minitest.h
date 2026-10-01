#pragma once

// 极简测试框架（googletest 在本环境无法安装时的替代品）。
// 提供 TEST / EXPECT_* 宏，行为对齐 googletest 的核心用法，便于日后无痛切换。

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace minitest {

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
  static std::vector<TestCase> r;
  return r;
}

struct Registrar {
  Registrar(const std::string& name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
  }
};

inline int& failureCount() {
  static int c = 0;
  return c;
}
inline std::string& currentTest() {
  static std::string s;
  return s;
}

inline void reportFailure(const std::string& expr, const std::string& file, int line) {
  ++failureCount();
  std::cerr << "  [FAIL] " << currentTest() << ": " << expr << " (" << file << ":" << line
            << ")\n";
}

template <typename T>
inline void checkTrue(const T& cond, const char* expr, const char* file, int line) {
  if (!cond) reportFailure(std::string("EXPECT_TRUE(") + expr + ")", file, line);
}

template <typename A, typename B>
inline void checkEq(const A& a, const B& b, const char* ea, const char* eb, const char* file,
                    int line) {
  if (!(a == b)) {
    std::ostringstream oss;
    oss << "EXPECT_EQ(" << ea << ", " << eb << ")";
    reportFailure(oss.str(), file, line);
  }
}

template <typename A, typename B>
inline void checkNe(const A& a, const B& b, const char* ea, const char* eb, const char* file,
                    int line) {
  if (a == b) {
    std::ostringstream oss;
    oss << "EXPECT_NE(" << ea << ", " << eb << ")";
    reportFailure(oss.str(), file, line);
  }
}

inline void checkStrEq(const std::string& a, const std::string& b, const char* ea, const char* eb,
                       const char* file, int line) {
  if (a != b) {
    std::ostringstream oss;
    oss << "EXPECT_STREQ(" << ea << ", " << eb << ") got '" << a << "' vs '" << b << "'";
    reportFailure(oss.str(), file, line);
  }
}

inline int runAll() {
  int passed = 0;
  for (auto& t : registry()) {
    currentTest() = t.name;
    const int before = failureCount();
    t.fn();
    if (failureCount() == before) {
      ++passed;
      std::cout << "[PASS] " << t.name << "\n";
    }
  }
  std::cout << passed << "/" << registry().size() << " tests passed\n";
  return failureCount() == 0 ? 0 : 1;
}

}  // namespace minitest

#define MINITEST_CONCAT_IMPL(a, b) a##b
#define MINITEST_CONCAT(a, b) MINITEST_CONCAT_IMPL(a, b)
#define MINITEST_STR_IMPL(x) #x
#define MINITEST_STR(x) MINITEST_STR_IMPL(x)

#define TEST(name)                                                              \
  static void MINITEST_CONCAT(minitest_fn_, __LINE__)();                        \
  static ::minitest::Registrar MINITEST_CONCAT(minitest_reg_, __LINE__)(        \
      MINITEST_STR(name), MINITEST_CONCAT(minitest_fn_, __LINE__));             \
  static void MINITEST_CONCAT(minitest_fn_, __LINE__)()

#define EXPECT_TRUE(cond) ::minitest::checkTrue((cond), #cond, __FILE__, __LINE__)
#define EXPECT_FALSE(cond) ::minitest::checkTrue(!(cond), "!(" #cond ")", __FILE__, __LINE__)
#define EXPECT_EQ(a, b) ::minitest::checkEq((a), (b), #a, #b, __FILE__, __LINE__)
#define EXPECT_NE(a, b) ::minitest::checkNe((a), (b), #a, #b, __FILE__, __LINE__)
#define EXPECT_STREQ(a, b) ::minitest::checkStrEq((a), (b), #a, #b, __FILE__, __LINE__)

#define MINITEST_MAIN() \
  int main() { return ::minitest::runAll(); }
