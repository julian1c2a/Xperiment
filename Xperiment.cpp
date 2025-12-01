#include <cstdint>
#include <iostream>

enum class ParseError {
    Success,
    InvalidCharacter,
    Overflow,
    Empty
};

template<typename E>
struct Unexpected {
    E error;

    constexpr explicit Unexpected(E e) noexcept : error(e) {}
};

template<typename T, typename E = ParseError>
struct Expected {
private:
    T val;
    E err;
    bool is_valid;

public:
    constexpr Expected(T v) noexcept
        : val(v), err(E::Success), is_valid(true) {
    }

    constexpr Expected(E e) noexcept
        : val(0), err(e), is_valid(false) {
    }

    // Constructor para Unexpected
    constexpr Expected(Unexpected<E> u) noexcept
        : val(0), err(u.error), is_valid(false) {
    }

    // Acceso al valor
    constexpr T value() const noexcept {
        return val;
    }

    // Acceso al error
    constexpr E error() const noexcept {
        return err;
    }

    // Verificar si es válido
    constexpr bool has_value() const noexcept {
        return is_valid;
    }

    // Operador bool para conversión implícita
    constexpr explicit operator bool() const noexcept {
        return is_valid;
    }

    // Operador * para acceso al valor (como std::expected)
    constexpr T operator*() const noexcept {
        return val;
    }
};

constexpr Expected<std::uint64_t> parse(const char* const str) noexcept {
    if (str == nullptr || str[0] == '\0') {
        return Unexpected<ParseError>(ParseError::Empty);
    }

    std::uint64_t result = 0;
    int index = 0;

    while (str[index] != '\0') {
        char c = str[index];

        if (c < '0' || c > '9') {
            return Unexpected<ParseError>(ParseError::InvalidCharacter);
        }

        std::uint64_t digit = c - '0';

        // Verificar overflow
        const std::uint64_t max_uint64 = 18446744073709551615ULL;
        if (result > max_uint64 / 10) {
            return Unexpected<ParseError>(ParseError::Overflow);
        }

        result *= 10;

        if (result > max_uint64 - digit) {
            return Unexpected<ParseError>(ParseError::Overflow);
        }

        result += digit;
        index++;
    }

    return Expected<std::uint64_t>(result);
}

struct Xperiment {
    const std::uint64_t valor;
    const ParseError error;
    const bool success;

    constexpr Xperiment(const char* const experimentName) noexcept
        : valor(parse(experimentName).value()),
        error(parse(experimentName).error()),
        success(parse(experimentName).has_value()) {
    }
};

constexpr Xperiment exp1("123");
constexpr Xperiment exp2("456789");
constexpr Xperiment exp3("18446744073709551615"); // max uint64_t

static_assert(exp1.success&& exp1.valor == 123, "Parse '123' should succeed with value 123");
static_assert(exp2.success&& exp2.valor == 456789, "Parse '456789' should succeed with value 456789");
static_assert(exp3.success&& exp3.valor == 18446744073709551615ULL, "Parse max uint64_t should succeed");

// Test error cases
constexpr Xperiment expError1("");
constexpr Xperiment expError2("12a34");
constexpr Xperiment expError3("18446744073709551616"); // overflow

static_assert(!expError1.success&& expError1.error == ParseError::Empty, "Empty string should fail");
static_assert(!expError2.success&& expError2.error == ParseError::InvalidCharacter, "Invalid character should fail");
static_assert(!expError3.success&& expError3.error == ParseError::Overflow, "Overflow should fail");

// Helper para imprimir el resultado en tiempo de ejecución
const char* parseErrorToString(ParseError e) noexcept {
    switch (e) {
    case ParseError::Success: return "Success";
    case ParseError::InvalidCharacter: return "InvalidCharacter";
    case ParseError::Overflow: return "Overflow";
    case ParseError::Empty: return "Empty";
    }
    return "Unknown";
}

int main() {
    std::cout << "exp1: success=" << exp1.success << " value=" << exp1.valor << "\n";
    std::cout << "exp2: success=" << exp2.success << " value=" << exp2.valor << "\n";
    std::cout << "exp3: success=" << exp3.success << " value=" << exp3.valor << "\n";

    std::cout << "expError1: success=" << expError1.success << " error=" << parseErrorToString(expError1.error) << "\n";
    std::cout << "expError2: success=" << expError2.success << " error=" << parseErrorToString(expError2.error) << "\n";
    std::cout << "expError3: success=" << expError3.success << " error=" << parseErrorToString(expError3.error) << "\n";

    return 0;
}