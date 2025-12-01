#include <cstdint>
#include <iostream>
#include <type_traits>
#include <limits>

#include "expected_cpp14.hpp"

enum class ParseError {
    InvalidCharacter,       // Carácter inválido encontrado
    BlankInterDigits,       // Espacios en blanco entre dígitos
    Overflow,               // Overflow al parsear número
    Empty,                  // No se encontró ningún carácter válido
    InvalidPrefix,          // No es "d" o "dig"
    MissingDelimiter,       // No se encuentra "#" o "["
    EmptyDigit,             // Dígito vacío    
    MismatchedDelimiter,    // "#" no coincide con "#", "[" no coincide con "]"
    InvalidDigit,           // Error en parsing del dígito
    MissingB,               // No se encuentra "B"
    InvalidBase,            // Error en parsing de la base
    EmptyBase,              // Base vacía
    BlankInterDigitsOfBase, // Espacios en blanco entre dígitos de la base
    BaseOutOfRange,         // base-1 > uint32_max
    UnknownError            // Error desconocido
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
