#include "gtest/gtest.h"
#include "../huffman-lib/huffman.h"
#include <array>
#include <bitset>
#include <random>
#include <sstream>
#include <vector>

template<typename Coll>
void compress_collection(Coll const& c, double expect_ratio) {
  using T = typename Coll::value_type;
  std::stringstream s1, s2, s3;
  for (T const& x : c) {
    s1 << x << ' ';
  }
  huffman::encode(s1, s2);
  huffman::decode(s2, s3);
  EXPECT_EQ(s1.str(), s3.str());
  EXPECT_GE(s1.str().size(), expect_ratio * s2.str().size());
}

TEST(correctness, empty_stream) {
  std::stringstream s1, s2, s3;
  EXPECT_NO_THROW(huffman::encode(s1, s2));
  EXPECT_NO_THROW(huffman::decode(s2, s3));
  EXPECT_TRUE(s3.str().empty());
}

TEST(correctness, decode_empty) {
  std::stringstream s1, s2;
  EXPECT_THROW(huffman::decode(s1, s2), std::invalid_argument);
}

TEST(correctness, corrupted_ignore_bits_value) {
  std::stringstream s1, s2, s3, s4;
  const std::string msg = "test message";
  s1 << msg;
  huffman::encode(s1, s2);
  // copy code lengths
  for (size_t i = 0; i < 256; i++) {
    char c;
    s2 >> c;
    s3 << c;
  }
  s2.get();
  s3 << std::numeric_limits<char>::max();
  {
    char c;
    while (s2 >> c) {
      s3 << c;
    }
  }
  EXPECT_THROW(huffman::decode(s3, s4), std::invalid_argument);
}

TEST(correctness, decode_random_bytes) {
  std::stringstream s1, s2;
  constexpr size_t N = 500;
  std::default_random_engine eng(42);
  std::uniform_int_distribution<char> random_char(std::numeric_limits<char>::min(),
                                                  std::numeric_limits<char>::max());
  for (size_t i = 0; i < N; i++) {
    s1 << random_char(eng);
  }
  EXPECT_THROW(huffman::decode(s1, s2), std::invalid_argument);
}

TEST(correctness, single_char) {
  constexpr size_t N = 5000;
  std::stringstream s1, s2, s3;
  s1 << std::string(N, 'a');
  huffman::encode(s1, s2);
  huffman::decode(s2, s3);
  EXPECT_EQ(s1.str(), s3.str());
  EXPECT_GE(s1.str().size(), 5 * s2.str().size());
}

TEST(correctness, distinct_chars) {
  constexpr size_t N = 5000;
  std::stringstream s1, s2, s3;
  {
    unsigned char cur_ch = '\0';
    for (size_t i = 0; i < N; i++) {
      s1 << cur_ch++;
    }
  }
  huffman::encode(s1, s2);
  huffman::decode(s2, s3);
  EXPECT_EQ(s1.str(), s3.str());
}

TEST(correctness, random_bytes) {
  constexpr size_t TEST_NUM = 100;
  constexpr size_t TEST_LENGTH = 1'000;
  std::default_random_engine eng(42);
  std::uniform_int_distribution<char> random_char(std::numeric_limits<char>::min(),
                                                  std::numeric_limits<char>::max());
  for (size_t i = 0; i < TEST_NUM; i++) {
    std::stringstream s1, s2, s3;
    for (size_t j = 0; j < TEST_LENGTH; j++) {
      s1 << random_char(eng);
    }
    s1.exceptions(std::stringstream::badbit);
    huffman::encode(s1, s2);
    huffman::decode(s2, s3);
    EXPECT_EQ(s1.str(), s3.str());
  }
}

TEST(compression_ratio, fibonacci) {
  constexpr size_t N = 100'000;
  std::array<size_t, N> fib;
  fib[0] = fib[1] = 1;
  for (size_t i = 2; i < N; i++) {
    fib[i] = fib[i - 1] + fib[i - 2];
  }
  compress_collection(fib, 2);
}

TEST(compression_ratio, primes) {
  constexpr size_t N = 100'000;
  std::bitset<N> not_prime;
  std::vector<size_t> primes;
  for (size_t i = 2; i < N; i++) {
    if (!not_prime[i]) {
      primes.emplace_back(i);
    }
    for (size_t j = i * i; j < N; j += i) {
      not_prime[j] = true;
    }
  }
  compress_collection(primes, 2);
}

TEST(compression_ratio, small_range_random) {
  constexpr size_t N = 100'000;
  std::default_random_engine eng(42);
  std::uniform_int_distribution<char> random_char('a', 'd');
  std::string s;
  for (size_t i = 0; i < N; i++) {
    s.push_back(random_char(eng));
  }
  compress_collection(s, 3.5);
}

TEST(performance, single_char) {
  constexpr size_t N = 100'000'000;
  std::stringstream s1, s2, s3;
  s1 << std::string(N, 'a');
  huffman::encode(s1, s2);
  huffman::decode(s2, s3);
}

TEST(performance, many_small_streams) {
  constexpr size_t TEST_NUMBER = 10'000;
  constexpr size_t TEST_LENGTH = 100;
  std::default_random_engine eng(42);
  std::uniform_int_distribution<char> random_char(std::numeric_limits<char>::min(),
                                                  std::numeric_limits<char>::max());
  for (size_t i = 0; i < TEST_NUMBER; i++) {
    std::stringstream s1, s2, s3;
    for (size_t j = 0; j < TEST_LENGTH; j++) {
      s1 << random_char(eng);
    }
    huffman::encode(s1, s2);
    huffman::decode(s2, s3);
  }
}

TEST(performance, random_bytes) {
  constexpr size_t N = 100'000'00;
  std::stringstream s1, s2, s3;
  std::default_random_engine eng(42);
  std::uniform_int_distribution<char> random_char(std::numeric_limits<char>::min(),
                                                  std::numeric_limits<char>::max());
  for (size_t i = 0; i < N; i++) {
    s1 << random_char(eng);
  }
  huffman::encode(s1, s2);
  huffman::decode(s2, s3);
}
