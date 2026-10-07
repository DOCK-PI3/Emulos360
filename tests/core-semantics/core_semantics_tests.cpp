// Contract tests for the real Xenia HIR constant folder. This program does not
// boot a title or modify the emulator's configuration, profiles or saves.
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>

#include "xenia/cpu/hir/value.h"

namespace {
using xe::cpu::hir::TypeName;
using xe::cpu::hir::Value;
using xe::cpu::hir::INT8_TYPE;
using xe::cpu::hir::INT16_TYPE;
using xe::cpu::hir::INT32_TYPE;
using xe::cpu::hir::INT64_TYPE;

struct Results {
  uint64_t checks = 0;
  uint64_t failures = 0;
  std::array<uint64_t, 4> failures_by_width{};
};

uint64_t NextBits(uint64_t& state) {
  state += UINT64_C(0x9E3779B97F4A7C15);
  auto value = state;
  value = (value ^ (value >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  value = (value ^ (value >> 27)) * UINT64_C(0x94D049BB133111EB);
  return value ^ (value >> 31);
}

// Independent oracle: logical guest bytes/shorts are the high-to-low parts
// of each guest word. The backend implements this using VEC128_B/W. It does
// not share the folder's u8/u16 indexing expression.
uint64_t Reference(const std::array<uint32_t, 4>& words, unsigned width,
                   uint8_t index) {
  const auto lane = static_cast<unsigned>(index) % (128U / width);
  if (width == 8) {
    return (words[lane / 4] >> (24U - (lane % 4) * 8U)) & 0xFFU;
  }
  if (width == 16) {
    return (words[lane / 2] >> (16U - (lane % 2) * 16U)) & 0xFFFFU;
  }
  if (width == 32) {
    return words[lane];
  }
  return static_cast<uint64_t>(words[lane * 2]) |
         (static_cast<uint64_t>(words[lane * 2 + 1]) << 32);
}

uint64_t Extract(const std::array<uint32_t, 4>& words, TypeName type,
                 uint8_t index) {
  xe::vec128_t vector{};
  std::memcpy(vector.u32, words.data(), sizeof(vector));
  Value input{};
  input.set_constant(vector);
  Value element{};
  element.set_constant(index);
  // Deliberately initialize the unused bytes differently. EXTRACT's index
  // is I8; reading wider union members must not determine the selected lane.
  element.constant.v128.u64[1] = UINT64_C(0xFEDCBA9876543210);
  Value result{};
  result.set_zero(type);
  result.Extract(&input, &element);
  switch (type) {
    case INT8_TYPE: return result.constant.u8;
    case INT16_TYPE: return result.constant.u16;
    case INT32_TYPE: return result.constant.u32;
    case INT64_TYPE: return result.constant.u64;
    default: return UINT64_MAX;
  }
}

void CheckVector(const std::array<uint32_t, 4>& words, bool all_indices,
                 Results& results) {
  constexpr std::array<unsigned, 4> widths{8, 16, 32, 64};
  constexpr std::array<TypeName, 4> types{INT8_TYPE, INT16_TYPE, INT32_TYPE,
                                       INT64_TYPE};
  for (size_t type_index = 0; type_index < types.size(); ++type_index) {
    const auto width = widths[type_index];
    const auto index_count = all_indices ? 256U : 128U / width;
    for (unsigned i = 0; i < index_count; ++i) {
      const auto index = static_cast<uint8_t>(i);
      const auto expected = Reference(words, width, index);
      const auto actual = Extract(words, types[type_index], index);
      ++results.checks;
      if (actual != expected) {
        if (results.failures < 8) {
          std::printf("FAIL I%u index=%u actual=0x%llX expected=0x%llX\n",
                      width, i, static_cast<unsigned long long>(actual),
                      static_cast<unsigned long long>(expected));
        }
        ++results.failures;
        ++results.failures_by_width[type_index];
      }
    }
  }
}
}  // namespace

int main(int argc, char** argv) {
  const bool all_indices = argc == 2 &&
      std::string_view(argv[1]) == "--all-indices";
  if (argc != 2 || (!all_indices && std::string_view(argv[1]) != "--lanes")) {
    std::fputs("usage: core_semantics_tests --lanes|--all-indices\n", stderr);
    return 2;
  }
  Results results;
  constexpr std::array<std::array<uint32_t, 4>, 5> fixtures{{
      {0x00010203U, 0x04050607U, 0x08090A0BU, 0x0C0D0E0FU},
      {0, 0, 0, 0},
      {UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX},
      {0x80000001U, 0x7FFFFFFEU, 0xFFFF0000U, 0x0000FFFFU},
      {0x12345678U, 0x9ABCDEF0U, 0x13579BDFU, 0x2468ACE0U},
  }};
  for (const auto& fixture : fixtures) {
    CheckVector(fixture, all_indices, results);
  }
  uint64_t random_state = UINT64_C(0xE36020261006);
  for (unsigned vector_index = 0; vector_index < 4096; ++vector_index) {
    std::array<uint32_t, 4> words{};
    for (auto& word : words) {
      word = static_cast<uint32_t>(NextBits(random_state));
    }
    CheckVector(words, all_indices, results);
  }
  std::printf("checks=%llu failures=%llu I8=%llu I16=%llu I32=%llu I64=%llu\n",
              static_cast<unsigned long long>(results.checks),
              static_cast<unsigned long long>(results.failures),
              static_cast<unsigned long long>(results.failures_by_width[0]),
              static_cast<unsigned long long>(results.failures_by_width[1]),
              static_cast<unsigned long long>(results.failures_by_width[2]),
              static_cast<unsigned long long>(results.failures_by_width[3]));
  return results.failures ? 1 : 0;
}
