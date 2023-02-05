#include "huffman.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <numeric>
#include <queue>

using namespace huffman;

namespace {
constexpr size_t CODE_WIDTH = 64;
using code_val_t = uint64_t;

size_t char_to_ind(char ch) {
  return static_cast<size_t>(ch - std::numeric_limits<char>::min());
}

char ind_to_char(size_t ind) {
  return static_cast<char>(std::numeric_limits<char>::min() + ind);
}

struct code {
  code_val_t value;
  uint8_t length;
};

using code_map = std::array<code, 1 << (8 * sizeof(char))>;

struct node {
  node(size_t count) : count(count) {}
  virtual void fill_code_lengths(code_map&, uint8_t depth) = 0;
  virtual ~node() {}
  size_t count;
};

struct inner_node : node {
  inner_node(node* left, node* right)
      : node(left->count + right->count), left(left), right(right) {}

  void fill_code_lengths(code_map& codes, uint8_t depth) override {
    left->fill_code_lengths(codes, depth + 1);
    right->fill_code_lengths(codes, depth + 1);
  }

  std::unique_ptr<node> left;
  std::unique_ptr<node> right;
};

struct leaf_node : node {
  leaf_node(size_t count, char ch) : node(count), ch(ch) {}

  void fill_code_lengths(code_map& codes, uint8_t depth) override {
    codes[char_to_ind(ch)].length = std::max((uint8_t)1, depth);
  }

  char ch;
};

struct nodes_greater {
  bool operator()(node* a, node* b) {
    return a->count > b->count;
  }
};

std::array<size_t, 256> fill_canonical_code_values(code_map& codes) {
  std::array<size_t, 256> p;
  std::iota(p.begin(), p.end(), 0);
  std::sort(p.begin(), p.end(), [&codes](size_t i, size_t j) -> bool {
    if (codes[i].length != codes[j].length) {
      return codes[i].length < codes[j].length;
    }
    return i < j;
  });

  codes[p[0]].value = 0;
  for (size_t i = 1; i < codes.size(); i++) {
    uint8_t cur_length{codes[p[i]].length};
    uint8_t prev_length{codes[p[i - 1]].length};
    if (prev_length == 0) {
      codes[p[i]].value = 0;
    } else {
      codes[p[i]].value = (codes[p[i - 1]].value + 1)
                       << (cur_length - prev_length);
    }
    if ((codes[p[i]].value >> codes[p[i]].length) != 0) {
      throw std::invalid_argument("code lengths corrupted");
    }
  }
  if (codes[p[p.size() - 2]].length != 0 &&
      codes[p.back()].value !=
          (static_cast<code_val_t>(1) << codes[p.back()].length) - 1) {
    throw std::invalid_argument("code lengths corrupted");
  }
  if (codes[p[p.size() - 2]].length == 0 && codes[p.back()].length > 1) {
    throw std::invalid_argument("code lengths corrupted");
  }
  return p;
}

template <typename T>
std::enable_if_t<sizeof(T) == 1, std::ostream&> write_byte(std::ostream& os,
                                                           T const& val) {
  return os.put(reinterpret_cast<char const&>(val));
}

template <typename T>
std::enable_if_t<sizeof(T) == 1, std::istream&> read_byte(std::istream& is,
                                                          T& val) {
  return is.get(reinterpret_cast<char&>(val));
}
} // namespace

void huffman::encode(std::istream& src, std::ostream& dst) {
  src.exceptions(std::ios::badbit);
  // count occurrences
  std::array<size_t, 1u << (8 * sizeof(char))> count{};
  char cur_ch;
  while (src.get(cur_ch)) {
    size_t ind{char_to_ind(cur_ch)};
    count[ind]++;
  }
  code_map codes;
  // huffman coding
  {
    std::priority_queue<node*, std::vector<node*>, nodes_greater> pq;
    for (size_t i = 0; i < count.size(); i++) {
      if (count[i] > 0) {
        pq.emplace(new leaf_node{count[i], ind_to_char(i)});
      } else {
        // unused characters have zero length code
        codes[i].length = 0;
      }
    }
    while (pq.size() > 1) {
      node* left = pq.top();
      pq.pop();
      node* right = pq.top();
      pq.pop();
      pq.push(new inner_node(left, right));
    }
    if (!pq.empty()) {
      // fill code_map
      node* root = pq.top();
      root->fill_code_lengths(codes, 0);
      delete root;
    }
    fill_canonical_code_values(codes);
  }

  for (code& c : codes) {
    write_byte(dst, c.length);
  }

  size_t msg_length{0};
  for (size_t i = 0; i < 256; i++) {
    msg_length += count[i] * codes[i].length;
  }
  uint8_t ignore_bits{static_cast<uint8_t>((8 - (msg_length % 8)) % 8)};
  write_byte(dst, ignore_bits);
  // encode message
  src.clear();
  src.seekg(0, std::ios::beg);
  code_val_t buff{0};
  unsigned buff_len{0};
  auto flush_buffer = [&dst, &buff, &buff_len]() -> void {
    while (buff_len > 0) {
      write_byte(dst, static_cast<uint8_t>(buff >> (CODE_WIDTH - 8)));
      buff <<= 8;
      buff_len = buff_len >= 8 ? buff_len - 8 : 0;
    }
  };
  while (src.get(cur_ch)) {
    code& cur_code = codes[char_to_ind(cur_ch)];
    if (buff_len + cur_code.length > CODE_WIDTH) {
      uint8_t can_write = CODE_WIDTH - buff_len;
      buff |= cur_code.value >> (cur_code.length - can_write);
      buff_len = CODE_WIDTH;
      flush_buffer();
      buff |= cur_code.value
           << (CODE_WIDTH - buff_len - cur_code.length + can_write);
      buff_len += cur_code.length - can_write;
    } else {
      buff |= cur_code.value << (CODE_WIDTH - buff_len - cur_code.length);
      buff_len += cur_code.length;
    }
  }
  if (buff_len > 0) {
    flush_buffer();
  }
}

void huffman::decode(std::istream& src, std::ostream& dst) {
  src.exceptions(std::ios::badbit);
  uint8_t max_length{0};
  code_map codes{};
  // read header
  for (code& c : codes) {
    read_byte(src, c.length);
    max_length = std::max(max_length, c.length);
  }
  uint8_t ignore_bits;
  read_byte(src, ignore_bits);

  if (src.eof()) {
    throw std::invalid_argument("corrupted input header");
  }

  if (ignore_bits > 8) {
    throw std::invalid_argument("bad ignore_bits value");
  }

  // fill codes
  std::array<size_t, 256> p = fill_canonical_code_values(codes);
  std::array<size_t, 256> inv_p;
  for (size_t i = 0; i < p.size(); i++) {
    inv_p[p[i]] = i;
  }
  std::array<size_t, 256> smallest_char;
  std::array<code_val_t const*, 256> smallest_code;
  std::array<code_val_t, 256> next_smallest_code;
  smallest_char[codes[p[0]].length] = p[0];
  smallest_code[codes[p[0]].length] = &codes[p[0]].value;
  for (size_t i = 1; i < codes.size(); i++) {
    uint8_t cur_length{codes[p[i]].length};
    uint8_t prev_length{codes[p[i - 1]].length};
    if (cur_length != prev_length) {
      smallest_char[cur_length] = p[i];
      smallest_code[cur_length] = &codes[p[i]].value;
      next_smallest_code[prev_length] = codes[p[i]].value
                                     << (CODE_WIDTH - 1 - cur_length);
    }
  }

  std::array<size_t, 256> start;
  std::fill(start.begin(), start.end(), 256);
  for (size_t i = 0; i < 256; i++) {
    auto& c = codes[i];
    if (codes[i].length == 0) {
      continue;
    }
    if (c.length >= 8) {
      uint8_t first_byte = c.value >> (c.length - 8);
      start[first_byte] =
          std::min(start[first_byte], static_cast<size_t>(c.length));
    } else {
      uint8_t first_byte = c.value << (8 - c.length);
      for (size_t i = 0; i < (1 << (8 - c.length)); i++) {
        start[first_byte | i] =
            std::min(start[first_byte | i], static_cast<size_t>(c.length));
      }
    }
  }

  next_smallest_code[max_length] = 1;
  next_smallest_code[max_length] <<= CODE_WIDTH - 1;
  // read message
  code cur_code{0, 0};
  auto read_buff = [&]() -> void {
    unsigned char cur_ch;
    while (cur_code.length + 8 <= CODE_WIDTH - 1 && read_byte(src, cur_ch)) {
      cur_code.value |= static_cast<code_val_t>(cur_ch)
                     << (CODE_WIDTH - 9 - cur_code.length);
      cur_code.length += 8;
    }
    cur_code.value &= (static_cast<code_val_t>(1) << (CODE_WIDTH - 1)) - 1;
  };
  read_buff();
  while (!src.eof() || cur_code.length > ignore_bits) {
    size_t cur_length = start[cur_code.value >> (CODE_WIDTH - 9)];
    assert(cur_length < 256);
    if (cur_length > 8) {
      while (cur_code.value >= next_smallest_code[cur_length]) {
        cur_length++;
      }
    }
    size_t d = (cur_code.value >> (CODE_WIDTH - 1 - cur_length)) -
             *smallest_code[cur_length];
    if (smallest_char[cur_length] + d >= 256) {
      throw std::invalid_argument("corrupted input message");
    }
    dst.put(ind_to_char(p[inv_p[smallest_char[cur_length]] + d]));
    cur_code.length -= cur_length;
    cur_code.value <<= cur_length;
    read_buff();
  }
}
