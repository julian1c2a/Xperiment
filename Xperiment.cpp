#include <cstdint>
#include <iostream>
#include <type_traits>
#include <limits>
#include "expected_cpp14.hpp"
#include "ParseError.hpp"

struct Xperiment {
    const Expected<std::uint64_t, ParseError> result;

    constexpr Xperiment(const char* const experimentName) noexcept
        : result(parse(experimentName)) {}
    
    // Optional: convenience accessors si realmente los necesitas
    constexpr std::uint64_t valor() const noexcept { return result.value(); }
    constexpr ParseError error() const noexcept { return result.error(); }
    constexpr bool success() const noexcept { return result.has_value(); }
};

// Tests originales
constexpr Xperiment experiment1("123");
constexpr Xperiment experiment2("456789");
constexpr Xperiment experiment3("18446744073709551615"); // max uint64_t

static_assert(experiment1.result && *experiment1.result == 123, "Parse '123' should succeed with value 123");
static_assert(experiment2.result && *experiment2.result == 456789, "Parse '456789' should succeed with value 456789");
static_assert(experiment3.result && *experiment3.result == 18446744073709551615ULL, "Parse max uint64_t should succeed");

// Tests para formato de dígito
constexpr DigitXperiment digitExp1("d#5#B3");              // 5 % 3 = 2
constexpr DigitXperiment digitExp2("dig [7] B 10");        // 7 % 10 = 7
constexpr DigitXperiment digitExp3("d  #  100  #  B  7");  // 100 % 7 = 2
constexpr DigitXperiment digitExp4("dig[15]B16");          // 15 % 16 = 15

static_assert(digitExp1.result && digitExp1.valor() == 2 && digitExp1.digit() == 5 && digitExp1.base() == 3, "d#5#B3 should work");
static_assert(digitExp2.result && digitExp2.valor() == 7 && digitExp2.digit() == 7 && digitExp2.base() == 10, "dig [7] B 10 should work");
static_assert(digitExp3.result && digitExp3.valor() == 2 && digitExp3.digit() == 100 && digitExp3.base() == 7, "d  #  100  #  B  7 should work");
static_assert(digitExp4.result && digitExp4.valor() == 15 && digitExp4.digit() == 15 && digitExp4.base() == 16, "dig[15]B16 should work");

// Test error cases para formato de dígito
constexpr DigitXperiment digitError1("x#5#B3");            // InvalidPrefix
constexpr DigitXperiment digitError2("d5B3");              // MissingDelimiter
constexpr DigitXperiment digitError3("d#5]B3");            // MismatchedDelimiter
constexpr DigitXperiment digitError4("d[5[B3");            // MismatchedDelimiter
constexpr DigitXperiment digitError5("d#5#C3");            // MissingB
constexpr DigitXperiment digitError6("d#5#B0");            // BaseOutOfRange (base-1 = -1)

static_assert(!digitError1.result && digitError1.error() == ParseError::InvalidPrefix, "x#5#B3 should fail with InvalidPrefix");
static_assert(!digitError2.result && digitError2.error() == ParseError::MissingDelimiter, "d5B3 should fail with MissingDelimiter");
static_assert(!digitError3.result && digitError3.error() == ParseError::MismatchedDelimiter, "d#5]B3 should fail with MismatchedDelimiter");
static_assert(!digitError4.result && digitError4.error() == ParseError::MismatchedDelimiter, "d[5[B3 should fail with MismatchedDelimiter");
static_assert(!digitError5.result && digitError5.error() == ParseError::MissingB, "d#5#C3 should fail with MissingB");
static_assert(!digitError6.result && digitError6.error() == ParseError::BaseOutOfRange, "d#5#B0 should fail with BaseOutOfRange");

// Test error cases originales
constexpr Xperiment experimentError1("");
constexpr Xperiment experimentError2("12a34");
constexpr Xperiment experimentError3("18446744073709551616"); // overflow
constexpr Xperiment experimentError4("12 34"); // blanks between digits
constexpr Xperiment experimentError5("  123  "); // leading/trailing whitespace (should succeed)

static_assert(!experimentError1.result && experimentError1.result.error() == ParseError::Empty, "Empty string should fail");
static_assert(!experimentError2.result && experimentError2.result.error() == ParseError::InvalidCharacter, "Invalid character should fail");
static_assert(!experimentError3.result && experimentError3.result.error() == ParseError::Overflow, "Overflow should fail");
static_assert(!experimentError4.result && experimentError4.result.error() == ParseError::BlankInterDigits, "Blank inter digits should fail");
static_assert(experimentError5.result && *experimentError5.result == 123, "Leading/trailing whitespace should succeed");

// Mantener la función original para runtime
constexpr Expected<DigitResult, ParseError> parse_digit_format(const char* const str) noexcept {
    return parse_digit_format_simple(str);
}

// Helper para imprimir el resultado en tiempo de ejecución
const char* parseErrorToString(ParseError e) noexcept {
    switch (e) {
    case ParseError::InvalidCharacter: return "InvalidCharacter";
    case ParseError::BlankInterDigits: return "BlankInterDigits";
    case ParseError::Overflow: return "Overflow";
    case ParseError::Empty: return "Empty";
    case ParseError::InvalidPrefix: return "InvalidPrefix";
    case ParseError::MissingDelimiter: return "MissingDelimiter";
    case ParseError::MismatchedDelimiter: return "MismatchedDelimiter";
    case ParseError::InvalidDigit: return "InvalidDigit";
    case ParseError::MissingB: return "MissingB";
    case ParseError::InvalidBase: return "InvalidBase";
    case ParseError::BaseOutOfRange: return "BaseOutOfRange";
    }
    return "Unknown";
}

int main() {
    std::cout << "=== Original Parse Tests ===\n";
    std::cout << "experiment1: success=" << experiment1.result.has_value() << " value=" << (experiment1.result ? *experiment1.result : 0) << "\n";
    std::cout << "experiment2: success=" << experiment2.result.has_value() << " value=" << (experiment2.result ? *experiment2.result : 0) << "\n";
    std::cout << "experiment3: success=" << experiment3.result.has_value() << " value=" << (experiment3.result ? *experiment3.result : 0) << "\n";

    std::cout << "\n=== Digit Format Parse Tests ===\n";
    std::cout << "d#5#B3: success=" << digitExp1.result.has_value() << " digit=" << (digitExp1.result.has_value() ? digitExp1.digit() : 0) 
              << " base=" << (digitExp1.result.has_value() ? digitExp1.base() : 0) << " result=" << (digitExp1.result.has_value() ? digitExp1.valor() : 0) << "\n";
    
    std::cout << "dig [7] B 10: success=" << digitExp2.result.has_value() << " digit=" << (digitExp2.result.has_value() ? digitExp2.digit() : 0) 
              << " base=" << (digitExp2.result.has_value() ? digitExp2.base() : 0) << " result=" << (digitExp2.result.has_value() ? digitExp2.valor() : 0) << "\n";
    
    std::cout << "d  #  100  #  B  7: success=" << digitExp3.result.has_value() << " digit=" << (digitExp3.result.has_value() ? digitExp3.digit() : 0) 
              << " base=" << (digitExp3.result.has_value() ? digitExp3.base() : 0) << " result=" << (digitExp3.result.has_value() ? digitExp3.valor() : 0) << "\n";

    std::cout << "\n=== Digit Format Error Tests ===\n";
    std::cout << "x#5#B3: success=" << digitError1.result.has_value() << " error=" << parseErrorToString(digitError1.error()) << "\n";
    std::cout << "d5B3: success=" << digitError2.result.has_value() << " error=" << parseErrorToString(digitError2.error()) << "\n";
    std::cout << "d#5]B3: success=" << digitError3.result.has_value() << " error=" << parseErrorToString(digitError3.error()) << "\n";

    // Test runtime parsing
    std::cout << "\n=== Runtime Digit Format Tests ===\n";
    auto rt1 = parse_digit_format("d#123#B256");
    if (rt1.has_value()) {
        std::cout << "d#123#B256: digit=" << rt1.value().digit << " base=" << rt1.value().base << " result=" << rt1.value().result << "\n";
    }

    auto rt2 = parse_digit_format("dig[999]B1000");
    if (rt2.has_value()) {
        std::cout << "dig[999]B1000: digit=" << rt2.value().digit << " base=" << rt2.value().base << " result=" << rt2.value().result << "\n";
    }

    auto rt3 = parse_digit_format("d #42# B 8");
    if (rt3.has_value()) {
        std::cout << "d #42# B 8: digit=" << rt3.value().digit << " base=" << rt3.value().base << " result=" << rt3.value().result << "\n";
    } else {
        std::cout << "d #42# B 8: failed with " << parseErrorToString(rt3.error()) << "\n";
    }

    return 0;
}