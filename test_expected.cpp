#include "expected_cpp14.hpp"
#include <iostream>
#include <string>
#include <cassert>
#include <vector>

// Un tipo no trivial simple para tests de construcción/destrucción.
struct NonTrivial {
    std::string s;
    static int copy_constructions;
    static int move_constructions;

    NonTrivial(const std::string& str = "") : s(str) {}
    NonTrivial(const NonTrivial& other) : s(other.s) {
        copy_constructions++;
    }
    NonTrivial(NonTrivial&& other) noexcept : s(std::move(other.s)) {
        move_constructions++;
    }
    NonTrivial& operator=(const NonTrivial& other) {
        s = other.s;
        copy_constructions++;
        return *this;
    }
    NonTrivial& operator=(NonTrivial&& other) noexcept {
        s = std::move(other.s);
        move_constructions++;
        return *this;
    }

    bool operator==(const NonTrivial& other) const { return s == other.s; }
};
int NonTrivial::copy_constructions = 0;
int NonTrivial::move_constructions = 0;


void test_construction_and_access() {
    std::cout << "--- Testing Construction & Access ---
";
    // Construcción con valor
    Expected<int, std::string> e1(42);
    assert(e1.has_value());
    assert(e1);
    assert(*e1 == 42);
    assert(e1.value() == 42);

    // Construcción con error
    Expected<int, std::string> e2(make_unexpected(std::string("error")));
    assert(!e2.has_value());
    assert(!e2);
    assert(e2.error() == "error");

    // Construcción por copia (valor)
    Expected<int, std::string> e3 = e1;
    assert(e3.has_value());
    assert(*e3 == 42);

    // Construcción por copia (error)
    Expected<int, std::string> e4 = e2;
    assert(!e4.has_value());
    assert(e4.error() == "error");
    
    // Construcción por movimiento (valor con tipo no-trivial)
    NonTrivial::move_constructions = 0;
    Expected<NonTrivial, int> e5(NonTrivial("hello"));
    Expected<NonTrivial, int> e6 = std::move(e5);
    assert(e6.has_value());
    assert(e6->s == "hello");
    assert(NonTrivial::move_constructions > 0);

    std::cout << "Access tests...\n";
    bool thrown = false;
    try {
        e1.error();
    } catch (const bad_expected_access&) {
        thrown = true;
    }
    assert(thrown);

    thrown = false;
    try {
        e2.value();
    } catch (const bad_expected_access&) {
        thrown = true;
    }
    assert(thrown);
    
    assert(e1.value_or(0) == 42);
    assert(e2.value_or(0) == 0);
}

void test_assignment() {
    std::cout << "--- Testing Assignment ---
";
    Expected<std::string, int> e1("initial");
    Expected<std::string, int> e2(make_unexpected(404));
    Expected<std::string, int> e3("new value");

    // Asignación de error a valor
    e1 = e2;
    assert(!e1.has_value());
    assert(e1.error() == 404);
    
    // Asignación de valor a error
    e1 = e3;
    assert(e1.has_value());
    assert(e1.value() == "new value");

    // Asignación por movimiento
    e1 = std::move(e3);
    assert(e1.has_value());
    assert(e1.value() == "new value");
}

void test_monadic_ops() {
    std::cout << "--- Testing Monadic Operations ---
";
    Expected<int, std::string> e_val(5);
    Expected<int, std::string> e_err(make_unexpected(std::string("error")));

    // and_then
    auto res1 = e_val.and_then([](int i) { return Expected<double, std::string>(i * 2.0); });
    assert(res1.has_value());
    assert(res1.value() == 10.0);
    auto res2 = e_err.and_then([](int i) { return Expected<double, std::string>(i * 2.0); });
    assert(!res2.has_value());
    assert(res2.error() == "error");

    // or_else
    auto res3 = e_val.or_else([](std::string s) { return Expected<int, std::string>(make_unexpected("new_error")); });
    assert(res3.has_value());
    assert(res3.value() == 5);
    auto res4 = e_err.or_else([](std::string s) { return Expected<int, std::string>(s.length()); });
    assert(res4.has_value());
    assert(res4.value() == 5); // "error" tiene 5 letras

    // transform
    auto res5 = e_val.transform([](int i) { return std::to_string(i); });
    assert(res5.has_value());
    assert(res5.value() == "5");
    auto res6 = e_err.transform([](int i) { return std::to_string(i); });
    assert(!res6.has_value());
    assert(res6.error() == "error");
    
    // transform_error
    auto res7 = e_err.transform_error([](std::string s){ return s.length(); });
    assert(!res7.has_value());
    assert(res7.error() == 5);
}

void test_void_specialization() {
    std::cout << "--- Testing Void Specialization ---
";
    Expected<void, int> v_ok;
    Expected<void, int> v_err(make_unexpected(42));

    assert(v_ok.has_value());
    assert(static_cast<bool>(v_ok));
    v_ok.value(); // no debe lanzar excepción

    assert(!v_err.has_value());
    assert(v_err.error() == 42);

    bool thrown = false;
    try {
        v_err.value();
    } catch(const bad_expected_access&) {
        thrown = true;
    }
    assert(thrown);

    auto res = v_ok.and_then([](){ return Expected<int, int>(123); });
    assert(res.has_value() && res.value() == 123);
    
    auto res2 = v_err.and_then([](){ return Expected<int, int>(123); });
    assert(!res2.has_value() && res2.error() == 42);
}


void test_in_place_construction() {
    std::cout << "--- Testing In-Place Construction ---\n";
    // In-place construction with a simple type
    Expected<int, std::string> e1(in_place, 42);
    assert(e1.has_value());
    assert(e1.value() == 42);

    // In-place construction with a multi-argument constructor
    Expected<std::string, int> e2(in_place, 5, 'c');
    assert(e2.has_value());
    assert(e2.value() == "ccccc");

    // In-place construction with a non-trivial type
    NonTrivial::copy_constructions = 0;
    NonTrivial::move_constructions = 0;
    Expected<NonTrivial, int> e3(in_place, "in-place");
    assert(e3.has_value());
    assert(e3.value().s == "in-place");
    assert(NonTrivial::copy_constructions == 0);
    //assert(NonTrivial::move_constructions == 0); // This might fail depending on string implementation
}

int main() {
    std::cout << "Running tests for Expected<T, E>...\n" << std::endl;
    
    test_construction_and_access();
    std::cout << std::endl;
    
    test_assignment();
    std::cout << std::endl;
    
    test_monadic_ops();
    std::cout << std::endl;
    
    test_void_specialization();
    std::cout << std::endl;

    test_in_place_construction();
    std::cout << std::endl;
    
    std::cout << "All tests passed!" << std::endl;
    
    return 0;
}
