#include <cstdint>
#include <iostream>
#include <type_traits>
#include <limits>

enum class ParseError {
    InvalidCharacter,
    BlankInterDigits,
    Overflow,
    Empty,
    // Nuevos errores para formato de dígito
    InvalidPrefix,        // No es "d" o "dig"
    MissingDelimiter,     // No se encuentra "#" o "["
    MismatchedDelimiter,  // "#" no coincide con "#", "[" no coincide con "]"
    InvalidDigit,         // Error en parsing del dígito
    MissingB,            // No se encuentra "B"
    InvalidBase,         // Error en parsing de la base
    BaseOutOfRange       // base-1 > uint32_max
};

// std::unexpected-compatible implementation for C++14
template<typename E>
struct Unexpected {
    E error;

    constexpr explicit Unexpected(E e) noexcept : error(e) {}
    
    // Allow conversion from compatible error types (like std::unexpected)
    template<typename G, typename = typename std::enable_if<!std::is_same<E, G>::value>::type>
    constexpr Unexpected(const Unexpected<G>& other) noexcept : error(other.error) {}
    
    // Copy constructor
    constexpr Unexpected(const Unexpected& other) = default;
    
    // Move constructor  
    constexpr Unexpected(Unexpected&& other) = default;
    
    // Assignment operators
    Unexpected& operator=(const Unexpected& other) = default;
    Unexpected& operator=(Unexpected&& other) = default;
    
    // Equality operators (C++23 std::unexpected has these)
    constexpr bool operator==(const Unexpected& other) const noexcept {
        return error == other.error;
    }
    
    constexpr bool operator!=(const Unexpected& other) const noexcept {
        return error != other.error;
    }
};

// Helper function similar to std::make_unexpected
template<typename E>
constexpr Unexpected<typename std::decay<E>::type> make_unexpected(E&& e) noexcept {
    return Unexpected<typename std::decay<E>::type>(static_cast<E&&>(e));
}

// Simplified Expected implementation for C++14/MSVC compatibility
template<typename T, typename E>
struct Expected {
private:
    T value_;
    E error_;
    bool has_value_;

public:
    // Value constructor
    template<typename U = T>
    constexpr Expected(U&& value) noexcept 
        : value_(static_cast<U&&>(value)), error_(), has_value_(true) {}

    // Error constructor
    constexpr Expected(E error) noexcept 
        : value_(), error_(error), has_value_(false) {}

    // Unexpected constructor
    constexpr Expected(Unexpected<E> unexpected) noexcept 
        : value_(), error_(unexpected.error), has_value_(false) {}

    // Template unexpected constructor for compatible types
    template<typename G>
    constexpr Expected(const Unexpected<G>& unexpected) noexcept 
        : value_(), error_(unexpected.error), has_value_(false) {}

    // Copy constructor
    constexpr Expected(const Expected& other) noexcept 
        : value_(other.value_), error_(other.error_), has_value_(other.has_value_) {}

    // Move constructor
    constexpr Expected(Expected&& other) noexcept 
        : value_(static_cast<T&&>(other.value_)), error_(other.error_), has_value_(other.has_value_) {}

    // Assignment operators
    Expected& operator=(const Expected& other) noexcept {
        if (this != &other) {
            value_ = other.value_;
            error_ = other.error_;
            has_value_ = other.has_value_;
        }
        return *this;
    }

    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            value_ = static_cast<T&&>(other.value_);
            error_ = other.error_;
            has_value_ = other.has_value_;
        }
        return *this;
    }

    // Observer methods
    constexpr bool has_value() const noexcept {
        return has_value_;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    // Value access
    constexpr T& value() noexcept {
        return value_;
    }

    constexpr const T& value() const noexcept {
        return value_;
    }

    constexpr T& operator*() noexcept {
        return value_;
    }

    constexpr const T& operator*() const noexcept {
        return value_;
    }

    constexpr T* operator->() noexcept {
        return &value_;
    }

    constexpr const T* operator->() const noexcept {
        return &value_;
    }

    // Error access
    constexpr E& error() noexcept {
        return error_;
    }

    constexpr const E& error() const noexcept {
        return error_;
    }

    // Value or default
    template<typename U>
    constexpr T value_or(U&& default_value) const noexcept {
        return has_value_ ? value_ : static_cast<T>(static_cast<U&&>(default_value));
    }

    // Monadic operations (C++14 compatible)
    template<typename F>
    constexpr auto and_then(F&& func) const -> decltype(func(value_)) {
        using ReturnType = decltype(func(value_));
        if (has_value_) {
            return func(value_);
        } else {
            return ReturnType(error_);
        }
    }

    template<typename F>
    constexpr auto or_else(F&& func) const -> Expected<T, E> {
        if (!has_value_) {
            return func(error_);
        } else {
            return *this;
        }
    }

    template<typename F>
    constexpr auto transform(F&& func) const -> Expected<decltype(func(value_)), E> {
        using ReturnType = Expected<decltype(func(value_)), E>;
        if (has_value_) {
            return ReturnType(func(value_));
        } else {
            return ReturnType(error_);
        }
    }

    // Transform error
    template<typename F>
    constexpr auto transform_error(F&& func) const -> Expected<T, decltype(func(error_))> {
        using ReturnType = Expected<T, decltype(func(error_))>;
        if (!has_value_) {
            return ReturnType(make_unexpected(func(error_)));
        } else {
            return ReturnType(value_);
        }
    }
};

// Specialization for void
template<typename E>
struct Expected<void, E> {
private:
    E error_;
    bool has_value_;

public:
    // Success constructor
    constexpr Expected() noexcept : error_(), has_value_(true) {}

    // Error constructor
    constexpr Expected(E error) noexcept : error_(error), has_value_(false) {}

    // Unexpected constructor
    constexpr Expected(Unexpected<E> unexpected) noexcept : error_(unexpected.error), has_value_(false) {}

    constexpr bool has_value() const noexcept {
        return has_value_;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    constexpr void value() const noexcept {
        // void return, nothing to do
    }

    constexpr E& error() noexcept {
        return error_;
    }

    constexpr const E& error() const noexcept {
        return error_;
    }
};

// Helper para skippear blancos
constexpr int skip_whitespace(const char* str, int index) noexcept {
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }
    return index;
}

// Parser de número sin blancos intermedios - versión simplificada para C++14
constexpr Expected<std::uint64_t, ParseError> parse_number_simple(const char* const str, int start_index, int& end_index) noexcept {
    if (str[start_index] < '0' || str[start_index] > '9') {
        end_index = start_index;
        return make_unexpected(ParseError::InvalidCharacter);
    }

    std::uint64_t result = 0;
    int index = start_index;
    bool found_digit = false;

    while (str[index] != '\0' && str[index] >= '0' && str[index] <= '9') {
        found_digit = true;
        std::uint64_t digit = str[index] - '0';

        // Verificar overflow
        const std::uint64_t max_uint64 = 18446744073709551615ULL;
        if (result > max_uint64 / 10) {
            end_index = index;
            return make_unexpected(ParseError::Overflow);
        }

        result *= 10;

        if (result > max_uint64 - digit) {
            end_index = index;
            return make_unexpected(ParseError::Overflow);
        }

        result += digit;
        index++;
    }

    end_index = index;
    if (!found_digit) {
        return make_unexpected(ParseError::Empty);
    }

    return Expected<std::uint64_t, ParseError>(result);
}

// Versión simplificada del parser de formato de dígito para MSVC C++14
constexpr Expected<DigitResult, ParseError> parse_digit_format_simple(const char* const str) noexcept {
    if (str == nullptr || str[0] == '\0') {
        return make_unexpected(ParseError::Empty);
    }

    int index = 0;
    
    // 1. Parsear prefix: "d" | "dig"
    if (str[index] == 'd') {
        index++; // consumir 'd'
        if (str[index] == 'i' && str[index + 1] == 'g') {
            index += 2; // consumir "ig"
        }
    } else {
        return make_unexpected(ParseError::InvalidPrefix);
    }

    // 2. Skip blancos manualmente
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }

    // 3. Parsear delimitador inicial: "#" | "[
    char opening_delimiter = str[index];
    if (opening_delimiter != '#' && opening_delimiter != '[') {
        return make_unexpected(ParseError::MissingDelimiter);
    }
    index++; // consumir delimitador

    // 4. Skip blancos
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }

    // 5. Parsear dígito - versión inline para evitar problemas con constexpr
    if (str[index] < '0' || str[index] > '9') {
        return make_unexpected(ParseError::InvalidDigit);
    }

    std::uint64_t digit = 0;
    bool found_digit = false;

    while (str[index] != '\0' && str[index] >= '0' && str[index] <= '9') {
        found_digit = true;
        std::uint64_t d = str[index] - '0';

        // Verificar overflow para dígito
        const std::uint64_t max_uint64 = 18446744073709551615ULL;
        if (digit > max_uint64 / 10) {
            return make_unexpected(ParseError::Overflow);
        }

        digit *= 10;

        if (digit > max_uint64 - d) {
            return make_unexpected(ParseError::Overflow);
        }

        digit += d;
        index++;
    }

    if (!found_digit) {
        return make_unexpected(ParseError::InvalidDigit);
    }

    // 6. Skip blancos
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }

    // 7. Parsear delimitador de cierre
    char expected_closing = (opening_delimiter == '#') ? '#' : ']';
    if (str[index] != expected_closing) {
        return make_unexpected(ParseError::MismatchedDelimiter);
    }
    index++; // consumir delimitador de cierre

    // 8. Skip blancos
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }

    // 9. Parsear "B"
    if (str[index] != 'B') {
        return make_unexpected(ParseError::MissingB);
    }
    index++; // consumir 'B'

    // 10. Skip blancos
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }

    // 11. Parsear base - versión inline
    if (str[index] < '0' || str[index] > '9') {
        return make_unexpected(ParseError::InvalidBase);
    }

    std::uint64_t base = 0;
    bool found_base_digit = false;

    while (str[index] != '\0' && str[index] >= '0' && str[index] <= '9') {
        found_base_digit = true;
        std::uint64_t b = str[index] - '0';

        // Verificar overflow para base
        const std::uint64_t max_uint64 = 18446744073709551615ULL;
        if (base > max_uint64 / 10) {
            return make_unexpected(ParseError::Overflow);
        }

        base *= 10;

        if (base > max_uint64 - b) {
            return make_unexpected(ParseError::Overflow);
        }

        base += b;
        index++;
    }

    if (!found_base_digit) {
        return make_unexpected(ParseError::InvalidBase);
    }

    // 12. Validar que base-1 <= uint32_max
    const std::uint64_t uint32_max = 4294967295ULL; // std::numeric_limits<std::uint32_t>::max()
    if (base == 0 || (base - 1) > uint32_max) {
        return make_unexpected(ParseError::BaseOutOfRange);
    }

    // 13. Skip blancos finales y verificar que llegamos al final
    while (str[index] == ' ' || str[index] == '\t' || str[index] == '\n' || str[index] == '\r') {
        index++;
    }
    if (str[index] != '\0') {
        return make_unexpected(ParseError::InvalidCharacter);
    }

    return Expected<DigitResult, ParseError>(DigitResult(digit, base));
}

struct Xperiment {
    const Expected<std::uint64_t, ParseError> result;

    constexpr Xperiment(const char* const experimentName) noexcept
        : result(parse(experimentName)) {}
    
    // Optional: convenience accessors si realmente los necesitas
    constexpr std::uint64_t valor() const noexcept { return result.value(); }
    constexpr ParseError error() const noexcept { return result.error(); }
    constexpr bool success() const noexcept { return result.has_value(); }
};

template<std::uint64_t B>
	requires (B >= 2 && B-1 <= std::numeric_limits<std::uint32_t>::max())
struct digit {
    std::uint32_t value;
    constexpr digit(std::uint64_t v) noexcept : value(static_cast<std::uint32_t>(v % B)) {}
};

template<std::uint64_t B>
	requires (B >= 2 && B - 1 <= std::numeric_limits<std::uint32_t>::max())
struct DigitXperiment {
    const Expected<digit<B>, ParseError> result;

    constexpr DigitXperiment(const char* const experimentName) noexcept
        : result(parse_digit_format_simple(experimentName)) {}
    
    // Convenience accessors
    constexpr std::uint32_t digit() const noexcept { return result.value().digit; }
    constexpr std::uint32_t base() const noexcept { return result.value().base; }
    constexpr std::uint32_t valor() const noexcept { return result.value().result; }
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