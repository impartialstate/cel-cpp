// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "internal/overflow.h"

#include <functional>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/time/time.h"
#include "internal/testing.h"

namespace cel::internal {
namespace {

using ::testing::HasSubstr;
using ::testing::ValuesIn;

template <typename T>
struct TestCase {
  std::string test_name;
  std::function<absl::StatusOr<T>()> op;
  absl::StatusOr<T> result;
};

template <typename T>
void ExpectResult(const T& test_case) {
  auto result = test_case.op();
  ASSERT_EQ(result.status().code(), test_case.result.status().code());
  if (result.ok()) {
    EXPECT_EQ(*result, *test_case.result);
  } else {
    EXPECT_THAT(result.status().message(),
                HasSubstr(test_case.result.status().message()));
  }
}

using IntTestCase = TestCase<int64_t>;
using CheckedIntResultTest = testing::TestWithParam<IntTestCase>;
TEST_P(CheckedIntResultTest, IntOperations) { ExpectResult(GetParam()); }

const IntTestCase kIntCases[] = {
    // Addition tests.
    {"OneAddOne",
     [] { return CheckedAdd(static_cast<int64_t>(1), static_cast<int64_t>(1)); },
     int64_t{2}},
    {"ZeroAddOne",
     [] { return CheckedAdd(static_cast<int64_t>(0), static_cast<int64_t>(1)); },
     int64_t{1}},
    {"ZeroAddMinusOne",
     [] {
       return CheckedAdd(static_cast<int64_t>(0), static_cast<int64_t>(-1));
     },
     int64_t{-1}},
    {"OneAddZero",
     [] { return CheckedAdd(static_cast<int64_t>(1), static_cast<int64_t>(0)); },
     int64_t{1}},
    {"MinusOneAddZero",
     [] {
       return CheckedAdd(static_cast<int64_t>(-1), static_cast<int64_t>(0));
     },
     int64_t{-1}},
    {"OneAddIntMax",
     [] { return CheckedAdd(int64_t{1}, std::numeric_limits<int64_t>::max()); },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},
    {"MinusOneAddIntMin",
     [] {
       return CheckedAdd(int64_t{-1}, std::numeric_limits<int64_t>::lowest());
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},

    // Subtraction tests.
    {"TwoSubThree",
     [] { return CheckedSub(static_cast<int64_t>(2), static_cast<int64_t>(3)); },
     int64_t{-1}},
    {"TwoSubZero",
     [] { return CheckedSub(static_cast<int64_t>(2), static_cast<int64_t>(0)); },
     int64_t{2}},
    {"ZeroSubTwo",
     [] { return CheckedSub(static_cast<int64_t>(0), static_cast<int64_t>(2)); },
     int64_t{-2}},
    {"MinusTwoSubThree",
     [] {
       return CheckedSub(static_cast<int64_t>(-2), static_cast<int64_t>(3));
     },
     int64_t{-5}},
    {"MinusTwoSubZero",
     [] {
       return CheckedSub(static_cast<int64_t>(-2), static_cast<int64_t>(0));
     },
     int64_t{-2}},
    {"ZeroSubMinusTwo",
     [] {
       return CheckedSub(static_cast<int64_t>(0), static_cast<int64_t>(-2));
     },
     int64_t{2}},
    {"IntMinSubIntMax",
     [] {
       return CheckedSub(std::numeric_limits<int64_t>::max(),
                         std::numeric_limits<int64_t>::lowest());
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},

    // Multiplication tests.
    {"TwoMulThree",
     [] { return CheckedMul(static_cast<int64_t>(2), static_cast<int64_t>(3)); },
     int64_t{6}},
    {"MinusTwoMulThree",
     [] {
       return CheckedMul(static_cast<int64_t>(-2), static_cast<int64_t>(3));
     },
     int64_t{-6}},
    {"MinusTwoMulMinusThree",
     [] {
       return CheckedMul(static_cast<int64_t>(-2), static_cast<int64_t>(-3));
     },
     int64_t{6}},
    {"TwoMulMinusThree",
     [] {
       return CheckedMul(static_cast<int64_t>(2), static_cast<int64_t>(-3));
     },
     int64_t{-6}},
    {"TwoMulIntMax",
     [] { return CheckedMul(int64_t{2}, std::numeric_limits<int64_t>::max()); },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},
    {"MinusOneMulIntMin",
     [] {
       return CheckedMul(int64_t{-1}, std::numeric_limits<int64_t>::lowest());
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},
    {"IntMinMulMinusOne",
     [] {
       return CheckedMul(std::numeric_limits<int64_t>::lowest(), int64_t{-1});
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},
    {"IntMinMulZero",
     [] {
       return CheckedMul(std::numeric_limits<int64_t>::lowest(), int64_t{0});
     },
     int64_t{0}},
    {"ZeroMulIntMin",
     [] {
       return CheckedMul(int64_t{0}, std::numeric_limits<int64_t>::lowest());
     },
     int64_t{0}},
    {"IntMaxMulZero",
     [] { return CheckedMul(std::numeric_limits<int64_t>::max(), int64_t{0}); },
     int64_t{0}},
    {"ZeroMulIntMax",
     [] { return CheckedMul(int64_t{0}, std::numeric_limits<int64_t>::max()); },
     int64_t{0}},

    // Division cases.
    {"ZeroDivOne",
     [] { return CheckedDiv(static_cast<int64_t>(0), static_cast<int64_t>(1)); },
     int64_t{0}},
    {"TenDivTwo",
     [] {
       return CheckedDiv(static_cast<int64_t>(10), static_cast<int64_t>(2));
     },
     int64_t{5}},
    {"TenDivMinusOne",
     [] {
       return CheckedDiv(static_cast<int64_t>(10), static_cast<int64_t>(-1));
     },
     int64_t{-10}},
    {"MinusTenDivMinusOne",
     [] {
       return CheckedDiv(static_cast<int64_t>(-10), static_cast<int64_t>(-1));
     },
     int64_t{10}},
    {"MinusTenDivTwo",
     [] {
       return CheckedDiv(static_cast<int64_t>(-10), static_cast<int64_t>(2));
     },
     int64_t{-5}},
    {"OneDivZero", [] { return CheckedDiv(int64_t{1}, int64_t{0}); },
     absl::StatusOr<int64_t>(absl::InvalidArgumentError("divide by zero"))},
    {"IntMinDivMinusOne",
     [] {
       return CheckedDiv(std::numeric_limits<int64_t>::lowest(), int64_t{-1});
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},

    // Modulus cases.
    {"ZeroModTwo",
     [] { return CheckedMod(static_cast<int64_t>(0), static_cast<int64_t>(2)); },
     int64_t{0}},
    {"TwoModTwo",
     [] { return CheckedMod(static_cast<int64_t>(2), static_cast<int64_t>(2)); },
     int64_t{0}},
    {"ThreeModTwo",
     [] { return CheckedMod(static_cast<int64_t>(3), static_cast<int64_t>(2)); },
     int64_t{1}},
    {"TwoModZero",
     [] { return CheckedMod(static_cast<int64_t>(2), static_cast<int64_t>(0)); },
     absl::StatusOr<int64_t>(absl::InvalidArgumentError("modulus by zero"))},
    {"IntMinModTwo",
     [] {
       return CheckedMod(std::numeric_limits<int64_t>::lowest(), int64_t{2});
     },
     int64_t{0}},
    {"IntMaxModMinusOne",
     [] { return CheckedMod(std::numeric_limits<int64_t>::max(), int64_t{-1}); },
     int64_t{0}},
    {"IntMinModMinusOne",
     [] {
       return CheckedMod(std::numeric_limits<int64_t>::lowest(), int64_t{-1});
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},

    // Negation cases.
    {"NegateOne", [] { return CheckedNegation(int64_t{1}); }, int64_t{-1}},
    {"NegateMinInt64",
     [] { return CheckedNegation(std::numeric_limits<int64_t>::lowest()); },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("integer overflow"))},

    // Numeric conversion cases for uint -> int, double -> int
    {"Uint64Conversion", [] { return CheckedUint64ToInt64(1UL); }, int64_t{1}},
    {"Uint32MaxConversion",
     [] {
       return CheckedUint64ToInt64(
           static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
     },
     std::numeric_limits<int64_t>::max()},
    {"Uint32MaxConversionError",
     [] {
       return CheckedUint64ToInt64(
           static_cast<uint64_t>(std::numeric_limits<uint64_t>::max()));
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"DoubleConversion", [] { return CheckedDoubleToInt64(100.1); },
     int64_t{100}},
    {"DoubleInt64MaxConversionError",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::max()));
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"DoubleInt64MaxMinus512Conversion",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::max() - 512));
     },
     std::numeric_limits<int64_t>::max() - 1023},
    {"DoubleInt64MaxMinus1024Conversion",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::max() - 1024));
     },
     std::numeric_limits<int64_t>::max() - 1023},
    {"DoubleInt64MinConversionError",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::lowest()));
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"DoubleInt64MinMinusOneConversionError",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::lowest()) - 1.0);
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"DoubleInt64MinMinus511ConversionError",
     [] {
       return CheckedDoubleToInt64(
           static_cast<double>(std::numeric_limits<int64_t>::lowest()) - 511.0);
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"InfiniteConversionError",
     [] {
       return CheckedDoubleToInt64(std::numeric_limits<double>::infinity());
     },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"NegRangeConversionError", [] { return CheckedDoubleToInt64(-1.0e99); },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
    {"PosRangeConversionError", [] { return CheckedDoubleToInt64(1.0e99); },
     absl::StatusOr<int64_t>(absl::OutOfRangeError("out of int64 range"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedIntMathTest, CheckedIntResultTest, ValuesIn(kIntCases),
    [](const testing::TestParamInfo<CheckedIntResultTest::ParamType>& info) {
      return info.param.test_name;
    });

using UintTestCase = TestCase<uint64_t>;
using CheckedUintResultTest = testing::TestWithParam<UintTestCase>;
TEST_P(CheckedUintResultTest, UnsignedOperations) { ExpectResult(GetParam()); }

const UintTestCase kUintCases[] = {
    // Addition tests.
    {"OneAddOne",
     [] {
       return CheckedAdd(static_cast<uint64_t>(1), static_cast<uint64_t>(1));
     },
     uint64_t{2}},
    {"ZeroAddOne",
     [] {
       return CheckedAdd(static_cast<uint64_t>(0), static_cast<uint64_t>(1));
     },
     uint64_t{1}},
    {"OneAddZero",
     [] {
       return CheckedAdd(static_cast<uint64_t>(1), static_cast<uint64_t>(0));
     },
     uint64_t{1}},
    {"OneAddIntMax",
     [] {
       return CheckedAdd(uint64_t{1}, std::numeric_limits<uint64_t>::max());
     },
     absl::StatusOr<uint64_t>(
         absl::OutOfRangeError("unsigned integer overflow"))},

    // Subtraction tests.
    {"OneSubOne",
     [] {
       return CheckedSub(static_cast<uint64_t>(1), static_cast<uint64_t>(1));
     },
     uint64_t{0}},
    {"ZeroSubOne",
     [] {
       return CheckedSub(static_cast<uint64_t>(0), static_cast<uint64_t>(1));
     },
     absl::StatusOr<uint64_t>(
         absl::OutOfRangeError("unsigned integer overflow"))},
    {"OneSubZero",
     [] {
       return CheckedSub(static_cast<uint64_t>(1), static_cast<uint64_t>(0));
     },
     uint64_t{1}},

    // Multiplication tests.
    {"OneMulOne",
     [] {
       return CheckedMul(static_cast<uint64_t>(1), static_cast<uint64_t>(1));
     },
     uint64_t{1}},
    {"ZeroMulOne",
     [] {
       return CheckedMul(static_cast<uint64_t>(0), static_cast<uint64_t>(1));
     },
     uint64_t{0}},
    {"OneMulZero",
     [] {
       return CheckedMul(static_cast<uint64_t>(1), static_cast<uint64_t>(0));
     },
     uint64_t{0}},
    {"TwoMulUintMax",
     [] {
       return CheckedMul(uint64_t{2}, std::numeric_limits<uint64_t>::max());
     },
     absl::StatusOr<uint64_t>(
         absl::OutOfRangeError("unsigned integer overflow"))},

    // Division tests.
    {"TwoDivTwo",
     [] {
       return CheckedDiv(static_cast<uint64_t>(2), static_cast<uint64_t>(2));
     },
     uint64_t{1}},
    {"TwoDivFour",
     [] {
       return CheckedDiv(static_cast<uint64_t>(2), static_cast<uint64_t>(4));
     },
     uint64_t{0}},
    {"OneDivZero",
     [] {
       return CheckedDiv(static_cast<uint64_t>(1), static_cast<uint64_t>(0));
     },
     absl::StatusOr<uint64_t>(absl::InvalidArgumentError("divide by zero"))},

    // Modulus tests.
    {"TwoModTwo",
     [] {
       return CheckedMod(static_cast<uint64_t>(2), static_cast<uint64_t>(2));
     },
     uint64_t{0}},
    {"TwoModFour",
     [] {
       return CheckedMod(static_cast<uint64_t>(2), static_cast<uint64_t>(4));
     },
     uint64_t{2}},
    {"OneModZero",
     [] {
       return CheckedMod(static_cast<uint64_t>(1), static_cast<uint64_t>(0));
     },
     absl::StatusOr<uint64_t>(absl::InvalidArgumentError("modulus by zero"))},

    // Conversion test cases for int -> uint, double -> uint.
    {"Int64Conversion", [] { return CheckedInt64ToUint64(int64_t{1}); },
     uint64_t{1}},
    {"Int64MaxConversion",
     [] {
       return CheckedInt64ToUint64(std::numeric_limits<int64_t>::max());
     },
     static_cast<uint64_t>(std::numeric_limits<int64_t>::max())},
    {"NegativeInt64ConversionError",
     [] { return CheckedInt64ToUint64(int64_t{-1}); },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"DoubleConversion", [] { return CheckedDoubleToUint64(100.1); },
     uint64_t{100}},
    {"DoubleUint64MaxConversionError",
     [] {
       return CheckedDoubleToUint64(
           static_cast<double>(std::numeric_limits<uint64_t>::max()));
     },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"DoubleUint64MaxMinus512ConversionError",
     [] {
       return CheckedDoubleToUint64(
           static_cast<double>(std::numeric_limits<uint64_t>::max() - 512));
     },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"DoubleUint64MaxMinus1024Conversion",
     [] {
       return CheckedDoubleToUint64(
           static_cast<double>(std::numeric_limits<uint64_t>::max() - 1024));
     },
     std::numeric_limits<uint64_t>::max() - 2047},
    {"InfiniteConversionError",
     [] {
       return CheckedDoubleToUint64(std::numeric_limits<double>::infinity());
     },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"NegConversionError", [] { return CheckedDoubleToUint64(-1.1); },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"NegRangeConversionError", [] { return CheckedDoubleToUint64(-1.0e99); },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
    {"PosRangeConversionError", [] { return CheckedDoubleToUint64(1.0e99); },
     absl::StatusOr<uint64_t>(absl::OutOfRangeError("out of uint64 range"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedUintMathTest, CheckedUintResultTest, ValuesIn(kUintCases),
    [](const testing::TestParamInfo<CheckedUintResultTest::ParamType>& info) {
      return info.param.test_name;
    });

using DurationTestCase = TestCase<absl::Duration>;
using CheckedDurationResultTest = testing::TestWithParam<DurationTestCase>;
TEST_P(CheckedDurationResultTest, DurationOperations) {
  ExpectResult(GetParam());
}

const DurationTestCase kDurationCases[] = {
    // Addition tests.
    {"OneSecondAddOneSecond",
     [] { return CheckedAdd(absl::Seconds(1), absl::Seconds(1)); },
     absl::StatusOr<absl::Duration>(absl::Seconds(2))},
    {"MaxDurationAddOneNano",
     [] {
       return CheckedAdd(absl::Nanoseconds(std::numeric_limits<int64_t>::max()),
                         absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"MinDurationAddMinusOneNano",
     [] {
       return CheckedAdd(
           absl::Nanoseconds(std::numeric_limits<int64_t>::lowest()),
           absl::Nanoseconds(-1));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfinityAddOneNano",
     [] { return CheckedAdd(absl::InfiniteDuration(), absl::Nanoseconds(1)); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"NegInfinityAddOneNano",
     [] { return CheckedAdd(-absl::InfiniteDuration(), absl::Nanoseconds(1)); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"OneSecondAddInfinity",
     [] { return CheckedAdd(absl::Nanoseconds(1), absl::InfiniteDuration()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"OneSecondAddNegInfinity",
     [] { return CheckedAdd(absl::Nanoseconds(1), -absl::InfiniteDuration()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},

    // Subtraction tests for duration - duration.
    {"OneSecondSubOneSecond",
     [] { return CheckedSub(absl::Seconds(1), absl::Seconds(1)); },
     absl::StatusOr<absl::Duration>(absl::ZeroDuration())},
    {"MinDurationSubOneSecond",
     [] {
       return CheckedSub(
           absl::Nanoseconds(std::numeric_limits<int64_t>::lowest()),
           absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfinitySubOneNano",
     [] { return CheckedSub(absl::InfiniteDuration(), absl::Nanoseconds(1)); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"NegInfinitySubOneNano",
     [] {
       return CheckedSub(-absl::InfiniteDuration(), absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"OneNanoSubInfinity",
     [] { return CheckedSub(absl::Nanoseconds(1), absl::InfiniteDuration()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"OneNanoSubNegInfinity",
     [] {
       return CheckedSub(absl::Nanoseconds(1), -absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},

    // Subtraction tests for time - time.
    {"TimeSubOneSecond",
     [] {
       return CheckedSub(absl::FromUnixSeconds(100), absl::FromUnixSeconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::Seconds(99))},
    {"TimeWithNanosPositive",
     [] {
       return CheckedSub(absl::FromUnixSeconds(2) + absl::Nanoseconds(1),
                         absl::FromUnixSeconds(1) - absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::Seconds(1) + absl::Nanoseconds(2))},
    {"TimeWithNanosNegative",
     [] {
       return CheckedSub(absl::FromUnixSeconds(1) + absl::Nanoseconds(1),
                         absl::FromUnixSeconds(2) + absl::Seconds(1) -
                             absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::Seconds(-2) + absl::Nanoseconds(2))},
    {"MinTimestampMinusOne",
     [] {
       return CheckedSub(
           absl::FromUnixSeconds(std::numeric_limits<int64_t>::lowest()),
           absl::FromUnixSeconds(1));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfinitePastSubOneSecond",
     [] { return CheckedSub(absl::InfinitePast(), absl::FromUnixSeconds(1)); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfiniteFutureSubOneMinusSecond",
     [] { return CheckedSub(absl::InfiniteFuture(), absl::FromUnixSeconds(-1)); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfiniteFutureSubInfinitePast",
     [] { return CheckedSub(absl::InfiniteFuture(), absl::InfinitePast()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"InfinitePastSubInfiniteFuture",
     [] { return CheckedSub(absl::InfinitePast(), absl::InfiniteFuture()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},

    // Negation cases.
    {"NegateOneSecond", [] { return CheckedNegation(absl::Seconds(1)); },
     absl::StatusOr<absl::Duration>(absl::Seconds(-1))},
    {"NegateMinDuration",
     [] {
       return CheckedNegation(
           absl::Nanoseconds(std::numeric_limits<int64_t>::lowest()));
     },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"NegateInfiniteDuration",
     [] { return CheckedNegation(absl::InfiniteDuration()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
    {"NegateNegInfiniteDuration",
     [] { return CheckedNegation(-absl::InfiniteDuration()); },
     absl::StatusOr<absl::Duration>(absl::OutOfRangeError("integer overflow"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedDurationMathTest, CheckedDurationResultTest,
    ValuesIn(kDurationCases),
    [](const testing::TestParamInfo<CheckedDurationResultTest::ParamType>& info) {
      return info.param.test_name;
    });

using TimeTestCase = TestCase<absl::Time>;
using CheckedTimeResultTest = testing::TestWithParam<TimeTestCase>;
TEST_P(CheckedTimeResultTest, TimeDurationOperations) {
  ExpectResult(GetParam());
}

const TimeTestCase kTimeCases[] = {
    // Addition tests.
    {"DateAddOneHourMinusOneMilli",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(3506),
                         absl::Hours(1) + absl::Milliseconds(-1));
     },
     absl::StatusOr<absl::Time>(absl::FromUnixSeconds(7106) +
                                absl::Milliseconds(-1))},
    {"DateAddOneHourOneNano",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(3506),
                         absl::Hours(1) + absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Time>(absl::FromUnixSeconds(7106) +
                                absl::Nanoseconds(1))},
    {"MaxIntAddOneSecond",
     [] {
       return CheckedAdd(
           absl::FromUnixSeconds(std::numeric_limits<int64_t>::max()),
           absl::Seconds(1));
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("integer overflow"))},
    {"MaxTimestampAddOneSecond",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(253402300799), absl::Seconds(1));
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"TimeWithNanosNegative",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(1) + absl::Nanoseconds(1),
                         absl::Nanoseconds(-999999999));
     },
     absl::StatusOr<absl::Time>(absl::FromUnixNanos(2))},
    {"TimeWithNanosPositive",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(1) + absl::Nanoseconds(999999999),
                         absl::Nanoseconds(999999999));
     },
     absl::StatusOr<absl::Time>(absl::FromUnixSeconds(2) +
                                absl::Nanoseconds(999999998))},
    {"SecondsAddInfinity",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(1) + absl::Nanoseconds(999999999),
                         absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"SecondsAddNegativeInfinity",
     [] {
       return CheckedAdd(absl::FromUnixSeconds(1) + absl::Nanoseconds(999999999),
                         -absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"InfiniteFutureAddNegativeInfinity",
     [] {
       return CheckedAdd(absl::InfiniteFuture(), -absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"InfinitePastAddInfinity",
     [] { return CheckedAdd(absl::InfinitePast(), absl::InfiniteDuration()); },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},

    // Subtraction tests.
    {"DateSubOneHour",
     [] { return CheckedSub(absl::FromUnixSeconds(3506), absl::Hours(1)); },
     absl::StatusOr<absl::Time>(absl::FromUnixSeconds(-94))},
    {"MinTimestampSubOneSecond",
     [] {
       return CheckedSub(absl::FromUnixSeconds(-62135596800), absl::Seconds(1));
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"MinIntSubOneViaNanos",
     [] {
       return CheckedSub(
           absl::FromUnixSeconds(std::numeric_limits<int64_t>::min()),
           absl::Nanoseconds(1));
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("integer overflow"))},
    {"MinTimestampSubOneViaNanosScaleOverflow",
     [] {
       return CheckedSub(
           absl::FromUnixSeconds(-62135596800) + absl::Nanoseconds(1),
           absl::Nanoseconds(999999999));
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("timestamp overflow"))},
    {"SecondsSubInfinity",
     [] {
       return CheckedSub(absl::FromUnixSeconds(1) + absl::Nanoseconds(999999999),
                         absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("integer overflow"))},
    {"SecondsSubNegInfinity",
     [] {
       return CheckedSub(absl::FromUnixSeconds(1) + absl::Nanoseconds(999999999),
                         -absl::InfiniteDuration());
     },
     absl::StatusOr<absl::Time>(absl::OutOfRangeError("integer overflow"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedTimeDurationMathTest, CheckedTimeResultTest, ValuesIn(kTimeCases),
    [](const testing::TestParamInfo<CheckedTimeResultTest::ParamType>& info) {
      return info.param.test_name;
    });

using ConvertInt64Int32TestCase = TestCase<int32_t>;
using CheckedConvertInt64Int32Test =
    testing::TestWithParam<ConvertInt64Int32TestCase>;
TEST_P(CheckedConvertInt64Int32Test, Conversions) { ExpectResult(GetParam()); }

const ConvertInt64Int32TestCase kConvertInt64Int32Cases[] = {
    {"SimpleConversion", [] { return CheckedInt64ToInt32(int64_t{1}); },
     int32_t{1}},
    {"Int32MaxConversion",
     [] {
       return CheckedInt64ToInt32(
           static_cast<int64_t>(std::numeric_limits<int32_t>::max()));
     },
     std::numeric_limits<int32_t>::max()},
    {"Int32MaxConversionError",
     [] {
       return CheckedInt64ToInt32(
           static_cast<int64_t>(std::numeric_limits<int64_t>::max()));
     },
     absl::StatusOr<int32_t>(absl::OutOfRangeError("out of int32 range"))},
    {"Int32MinConversion",
     [] {
       return CheckedInt64ToInt32(
           static_cast<int64_t>(std::numeric_limits<int32_t>::lowest()));
     },
     std::numeric_limits<int32_t>::lowest()},
    {"Int32MinConversionError",
     [] {
       return CheckedInt64ToInt32(
           static_cast<int64_t>(std::numeric_limits<int64_t>::lowest()));
     },
     absl::StatusOr<int32_t>(absl::OutOfRangeError("out of int32 range"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedConvertInt64Int32Test, CheckedConvertInt64Int32Test,
    ValuesIn(kConvertInt64Int32Cases),
    [](const testing::TestParamInfo<CheckedConvertInt64Int32Test::ParamType>&
           info) { return info.param.test_name; });

using ConvertUint64Uint32TestCase = TestCase<uint32_t>;
using CheckedConvertUint64Uint32Test =
    testing::TestWithParam<ConvertUint64Uint32TestCase>;
TEST_P(CheckedConvertUint64Uint32Test, Conversions) {
  ExpectResult(GetParam());
}

const ConvertUint64Uint32TestCase kConvertUint64Uint32Cases[] = {
    {"SimpleConversion", [] { return CheckedUint64ToUint32(uint64_t{1}); },
     uint32_t{1}},
    {"Uint32MaxConversion",
     [] {
       return CheckedUint64ToUint32(
           static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()));
     },
     std::numeric_limits<uint32_t>::max()},
    {"Uint32MaxConversionError",
     [] {
       return CheckedUint64ToUint32(
           static_cast<uint64_t>(std::numeric_limits<uint64_t>::max()));
     },
     absl::StatusOr<uint32_t>(absl::OutOfRangeError("out of uint32 range"))},
};

INSTANTIATE_TEST_SUITE_P(
    CheckedConvertUint64Uint32Test, CheckedConvertUint64Uint32Test,
    ValuesIn(kConvertUint64Uint32Cases),
    [](const testing::TestParamInfo<CheckedConvertUint64Uint32Test::ParamType>&
           info) { return info.param.test_name; });

}  // namespace
}  // namespace cel::internal
