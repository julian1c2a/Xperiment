#ifndef EXPECTED_CPP14_HPP
#define EXPECTED_CPP14_HPP

#include <type_traits>
#include <utility>

// Forward declaration of Unexpected
template<typename E>
struct Unexpected;

// Forward declaration of Expected
template<typename T, typename E>
struct Expected;


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
    union ValueStorage {
        T value_;
        E error_;
    };
    ValueStorage storage_;
    bool has_value_;

public:
    // Value constructor
    template<typename U = T>
    constexpr Expected(U&& value) noexcept 
        : storage_.value_(static_cast<U&&>(value)), storage_.error_(), has_value_(true) {}

    // Error constructor
    constexpr Expected(E error) noexcept 
        : storage_.value_(), storage_.error_(error), has_value_(false) {}
    // Unexpected constructor
    constexpr Expected(Unexpected<E> unexpected) noexcept 
        : storage_.value_(), storage_.error_(unexpected.error), has_value_(false) {}

    // Template unexpected constructor for compatible types
    template<typename G>
    constexpr Expected(const Unexpected<G>& unexpected) noexcept 
        : storage_.value_(), storage_.error_(unexpected.error), has_value_(false) {}

    // Copy constructor
    constexpr Expected(const Expected& other) noexcept 
        : storage_.value_(other.storage_.value_), storage_.error_(other.storage_.error_), has_value_(other.has_value_) {}

    // Move constructor
    constexpr Expected(Expected&& other) noexcept 
        : storage_.value_(static_cast<T&&>(other.storage_.value_)), storage_.error_(other.storage_.error_), has_value_(other.has_value_) {}

    // Assignment operators
    Expected& operator=(const Expected& other) noexcept {
        if (this != &other) {
            storage_.value_ = other.storage_.value_;
            storage_.error_ = other.storage_.error_;
            has_value_ = other.has_value_;
        }
        return *this;
    }

    Expected& operator=(Expected&& other) noexcept {
        if (this != &other) {
            storage_.value_ = static_cast<T&&>(other.storage_.value_);
            storage_.error_ = other.storage_.error_;
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
    constexpr auto transform(F&& func) const 
        -> Expected<decltype(func(value_)), E> 
    {
            using ReturnType = Expected<decltype(func(value_)), E>;
            if (has_value_) {
                return ReturnType(func(value_));
            } else {
                return ReturnType(error_);
            }
    }

    // Transform error
    template<typename F>
    constexpr auto transform_error(F&& func) const 
        -> Expected<T, decltype(func(error_))> 
    {
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

#endif // EXPECTED_CPP14_HPP
