#ifndef EXPECTED_CPP14_HPP
#define EXPECTED_CPP14_HPP

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <new> // For placement new

// --- Forward declarations ---
template<typename E>
struct Unexpected;

template<typename T, typename E>
struct Expected;

// --- Exception for bad access ---
class bad_expected_access : public std::logic_error {
public:
    explicit bad_expected_access(const char* what_arg) : std::logic_error(what_arg) {}
};

// --- Unexpected type ---
template<typename E>
struct Unexpected {
    E err;

    constexpr explicit Unexpected(E e) : err(std::move(e)) {}

    template<typename G, typename = typename std::enable_if<!std::is_same<E, G>::value>::type>
    constexpr Unexpected(const Unexpected<G>& other) : err(other.err) {}

    constexpr const E& error() const& noexcept { return err; }
    constexpr E& error() & noexcept { return err; }
    constexpr const E&& error() const&& noexcept { return std::move(err); }
    constexpr E&& error() && noexcept { return std::move(err); }

    constexpr bool operator==(const Unexpected& other) const { return err == other.err; }
    constexpr bool operator!=(const Unexpected& other) const { return err != other.err; }
};

// Helper for creating Unexpected
template<typename E>
constexpr Unexpected<typename std::decay<E>::type> make_unexpected(E&& e) {
    return Unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

// --- In-place helpers ---
struct in_place_t {
    explicit in_place_t() = default;
};
constexpr in_place_t in_place{};


// --- Main Expected class ---
template<typename T, typename E>
struct Expected {
private:
    union Storage {
        T val;
        E err;
        Storage() {}
        ~Storage() {}
    } storage_;

    bool has_val;

    void destroy() noexcept {
        if (has_val) {
            storage_.val.~T();
        } else {
            storage_.err.~E();
        }
    }

public:
    using value_type = T;
    using error_type = E;
    using unexpected_type = Unexpected<E>;

    // --- Constructors ---

    // Value constructor
    template<typename U = T,
             typename std::enable_if<
                std::is_constructible<T, U&&>::value &&
                !std::is_same<typename std::decay<U>::type, in_place_t>::value &&
                !std::is_same<typename std::decay<U>::type, Expected>::value &&
                !std::is_same<typename std::decay<U>::type, unexpected_type>::value
             >::type* = nullptr>
    constexpr Expected(U&& value) noexcept(std::is_nothrow_constructible<T, U&&>::value)
        : has_val(true) {
        new (&storage_.val) T(std::forward<U>(value));
    }
    
    // In-place value constructor
    template <class... Args, typename std::enable_if<std::is_constructible<T, Args...>::value>::type* = nullptr>
    explicit Expected(in_place_t, Args&&... args)
     : has_val(true) {
        new (&storage_.val) T(std::forward<Args>(args)...);
    }

    // Unexpected constructor
    template<typename G>
    constexpr Expected(const Unexpected<G>& unex) noexcept(std::is_nothrow_constructible<E, const G&>::value)
        : has_val(false) {
        new (&storage_.err) E(unex.error());
    }
    
    // Copy constructor
    Expected(const Expected& other) : has_val(other.has_val) {
        if (has_val) {
            new (&storage_.val) T(other.storage_.val);
        } else {
            new (&storage_.err) E(other.storage_.err);
        }
    }

    // Move constructor
    Expected(Expected&& other) noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_constructible<E>::value)
        : has_val(other.has_val) {
        if (has_val) {
            new (&storage_.val) T(std::move(other.storage_.val));
        } else {
            new (&storage_.err) E(std::move(other.storage_.err));
        }
    }

    // --- Destructor ---
    ~Expected() {
        destroy();
    }

    // --- Assignment ---
    Expected& operator=(const Expected& other) {
        if (this == &other) return *this;
        destroy();
        has_val = other.has_val;
        if (has_val) {
            new (&storage_.val) T(other.storage_.val);
        } else {
            new (&storage_.err) E(other.storage_.err);
        }
        return *this;
    }

    Expected& operator=(Expected&& other) noexcept(
        std::is_nothrow_move_assignable<T>::value && std::is_nothrow_move_constructible<T>::value &&
        std::is_nothrow_move_assignable<E>::value && std::is_nothrow_move_constructible<E>::value) {
        if (this == &other) return *this;
        destroy();
        has_val = other.has_val;
        if (has_val) {
            new (&storage_.val) T(std::move(other.storage_.val));
        } else {
            new (&storage_.err) E(std::move(other.storage_.err));
        }
        return *this;
    }

    // --- Observers ---
    constexpr bool has_value() const noexcept { return has_val; }
    constexpr explicit operator bool() const noexcept { return has_val; }

    // --- Accessors ---
    T& value() & {
        if (!has_val) throw bad_expected_access("expected does not have a value");
        return storage_.val;
    }
    const T& value() const & {
        if (!has_val) throw bad_expected_access("expected does not have a value");
        return storage_.val;
    }

    T* operator->() { return &value(); }
    const T* operator->() const { return &value(); }
    T& operator*() & { return value(); }
    const T& operator*() const & { return value(); }

    E& error() & {
        if (has_val) throw bad_expected_access("expected has a value");
        return storage_.err;
    }
    const E& error() const & {
        if (has_val) throw bad_expected_access("expected has a value");
        return storage_.err;
    }

    template<typename U>
    constexpr T value_or(U&& default_value) const & {
        return has_val ? storage_.val : static_cast<T>(std::forward<U>(default_value));
    }

    // --- Monadic operations ---
    template<typename F>
    auto and_then(F&& func) {
        using Ret = decltype(func(value()));
        if (has_value()) {
            return func(value());
        }
        return Ret(make_unexpected(error()));
    }

    template<typename F>
    auto or_else(F&& func) {
        using Ret = decltype(func(error()));
        if (has_value()) {
            return Ret(in_place, value());
        }
        return func(error());
    }

    template<typename F>
    auto transform(F&& func) {
        using RetT = decltype(func(value()));
        using Ret = Expected<RetT, E>;
        if (has_value()) {
            return Ret(in_place, func(value()));
        }
        return Ret(make_unexpected(error()));
    }
    
    template<typename F>
    auto transform_error(F&& func) {
        using RetE = decltype(func(error()));
        using Ret = Expected<T, RetE>;
        if (has_value()) {
            return Ret(in_place, value());
        }
        return Ret(make_unexpected(func(error())));
    }
};

// --- Specialization for void ---
template<typename E>
struct Expected<void, E> {
private:
    E err;
    bool has_val;

public:
    using value_type = void;
    using error_type = E;
    using unexpected_type = Unexpected<E>;
    
    constexpr Expected() noexcept : err(), has_val(true) {}
    constexpr Expected(const Unexpected<E>& unex) : err(unex.error()), has_val(false) {}
    constexpr Expected(Unexpected<E>&& unex) : err(std::move(unex.error())), has_val(false) {}
    
    Expected(const Expected& other) = default;
    Expected(Expected&& other) = default;
    Expected& operator=(const Expected& other) = default;
    Expected& operator=(Expected&& other) = default;

    constexpr bool has_value() const noexcept { return has_val; }
    constexpr explicit operator bool() const noexcept { return has_val; }

    void value() const {
        if (!has_val) throw bad_expected_access("expected does not have a value");
    }
    void operator*() const { value(); }

    E& error() & {
        if (has_val) throw bad_expected_access("expected has a value");
        return err;
    }
    const E& error() const & {
        if (has_val) throw bad_expected_access("expected has a value");
        return err;
    }

    // --- Monadic operations ---
    template<class F>
    auto and_then(F&& f) {
        using Ret = decltype(f());
        if(has_value()) {
            return f();
        }
        return Ret(make_unexpected(error()));
    }

    template<class F>
    Expected or_else(F&& f) {
        if(has_value()) {
            return *this;
        }
        return f(error());
    }

    template<class F>
    auto transform(F&& f) const {
        using RetT = decltype(f());
        using Ret = Expected<RetT, E>;
        if(has_value()) {
            return Ret(f());
        }
        return Ret(make_unexpected(error()));
    }

    template<class F>
    auto transform_error(F&& f) {
        using RetE = decltype(f(error()));
        using Ret = Expected<void, RetE>;
        if(has_value()) {
            return Ret();
        }
        return Ret(make_unexpected(f(error())));
    }
};

#endif // EXPECTED_CPP14_HPP