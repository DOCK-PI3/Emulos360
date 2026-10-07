// Regression gate for the actual x64 JIT, with the candidate's constant folder.
#include <array>
#include <cstdint>

#include "xenia/cpu/testing/util.h"

namespace {
using xe::cpu::hir::HIRBuilder;
using xe::cpu::hir::INT8_TYPE;
using xe::cpu::hir::INT16_TYPE;
using xe::cpu::hir::INT32_TYPE;
using xe::cpu::hir::INT64_TYPE;
using xe::cpu::hir::TypeName;
using xe::cpu::testing::LoadVR;
using xe::cpu::testing::StoreGPR;
using xe::cpu::testing::TestFunction;
using xe::cpu::ppc::PPCContext;

uint64_t ExpectedElement(const xe::vec128_t& vector, unsigned width,
                         unsigned index) {
  if (width == 8) {
    return (vector.u32[index / 4] >> (24U - (index % 4) * 8U)) & 0xFFU;
  }
  if (width == 16) {
    return (vector.u32[index / 2] >> (16U - (index % 2) * 16U)) & 0xFFFFU;
  }
  return vector.u32[index];
}
}  // namespace

TEST_CASE("EMULOS_EXTRACT_FOLD_MATCHES_REAL_JIT", "[emulos-extract]") {
  const std::array<xe::vec128_t, 3> inputs{
      xe::vec128i(0x00010203U, 0x04050607U, 0x08090A0BU, 0x0C0D0E0FU),
      xe::vec128i(0x12345678U, 0x9ABCDEF0U, 0x13579BDFU, 0x2468ACE0U),
      xe::vec128i(0x80000001U, 0x7FFFFFFEU, 0xFFFF0000U, 0x0000FFFFU)};
  constexpr std::array<TypeName, 3> types{INT8_TYPE, INT16_TYPE, INT32_TYPE};
  constexpr std::array<unsigned, 3> widths{8, 16, 32};
  for (const auto& input : inputs) {
    for (size_t part = 0; part < types.size(); ++part) {
      const auto type = types[part];
      const auto width = widths[part];
      for (unsigned index = 0; index < 128U / width; ++index) {
        const auto expected = ExpectedElement(input, width, index);
        INFO("width=" << width << " index=" << index);
        TestFunction test([input, type, index](HIRBuilder& b) {
          StoreGPR(b, 3, b.ZeroExtend(
              b.Extract(LoadVR(b, 4), static_cast<uint8_t>(index), type),
              INT64_TYPE));
          StoreGPR(b, 6, b.ZeroExtend(
              b.Extract(b.LoadConstantVec128(input),
                        static_cast<uint8_t>(index), type), INT64_TYPE));
          b.Return();
        });
        test.Run([&](PPCContext* context) { context->v[4] = input; },
                 [&](PPCContext* context) {
                   REQUIRE(context->r[3] == expected);
                   REQUIRE(context->r[6] == expected);
                   REQUIRE(context->r[3] == context->r[6]);
                 });
      }
    }
  }
}
