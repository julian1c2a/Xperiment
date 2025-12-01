#include <cstdint>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <new> // For placement new
#include <limits>

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
