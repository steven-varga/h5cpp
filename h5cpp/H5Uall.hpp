/*
 * Copyright (c) 2018-2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#pragma once

#include "H5Tmeta.hpp"
#include <array>
#include <string>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <random>
// Pretty-print machinery (operator<< overloads at the bottom of this file) needs:
#include <ostream>
#include <iterator>
#include <tuple>
#include <utility>

// ── shared engine ─────────────────────────────────────────────────────────────
// One Mersenne Twister per thread, seeded once from hardware entropy.
// All distributions below draw from this engine — no per-call reseeding.
namespace h5::impl {
    inline std::mt19937& rng() {
        static thread_local std::mt19937 engine{ std::random_device{}() };
        return engine;
    }
}

// ── character sets for string generation ─────────────────────────────────────
namespace h5::utils::string {
    template<class T> struct literal {};
    template<> struct literal<char> {
        constexpr static char value[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template<> struct literal<wchar_t> {
        constexpr static wchar_t value[] =
            L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template<> struct literal<char16_t> {
        constexpr static char16_t value[] =
            u"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };
    template<> struct literal<char32_t> {
        constexpr static char32_t value[] =
            U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    };

    template<class T>
    std::basic_string<T> random(size_t min_len, size_t max_len) {
        // sizeof-based length — safe for all char widths (fixes UB in old strlen call)
        constexpr auto& alpha       = literal<T>::value;
        constexpr size_t alpha_len  = sizeof(alpha) / sizeof(T) - 1;
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<size_t> char_dist(0, alpha_len - 1);
        size_t n = len_dist(impl::rng());
        std::basic_string<T> s;
        s.reserve(n);
        std::generate_n(std::back_inserter(s), n,
            [&]{ return alpha[char_dist(impl::rng())]; });
        return s;
    }
}

// ── pipe adaptor: dist | h5::take(n)  →  std::vector<value_type> ─────────────
namespace h5 {
    struct take_t { size_t n; };
    inline take_t take(size_t n) { return {n}; }

    template<class Dist>
    std::vector<typename Dist::value_type>
    operator|(Dist dist, take_t t) {
        std::vector<typename Dist::value_type> out;
        out.reserve(t.n);
        for (size_t i = 0; i < t.n; ++i)
            out.push_back(*dist);
        return out;
    }
}

// ── distribution types ────────────────────────────────────────────────────────
// Each type:
//   - holds its std::*_distribution<T> as a member (stateful, no per-call ctor)
//   - operator*()  →  single value
//   - operator|(take_t)  →  std::vector<value_type>  (via the free operator above)

namespace h5 {
    /** uniform<T>{lo, hi}  — integers and floats */
    template<class T>
    struct uniform {
        using value_type = T;
        using dist_t = std::conditional_t<std::is_integral_v<T>,
            std::uniform_int_distribution<T>,
            std::uniform_real_distribution<T>>;

        uniform(T lo = T{0},
                T hi = std::is_integral_v<T> ? std::numeric_limits<T>::max() : T{1})
            : dist_(lo, hi) {}

        value_type operator*() { return dist_(impl::rng()); }
    private:
        dist_t dist_;
    };

    /** normal<T>{mean, stddev}  — floating-point only */
    template<class T>
    struct normal {
        using value_type = T;
        static_assert(std::is_floating_point_v<T>,
            "h5::normal requires a floating-point type");

        normal(T mean = T{0}, T stddev = T{1}) : dist_(mean, stddev) {}
        value_type operator*() { return dist_(impl::rng()); }
    private:
        std::normal_distribution<T> dist_;
    };

    /** exponential<T>{lambda}  — floating-point only */
    template<class T>
    struct exponential {
        using value_type = T;
        static_assert(std::is_floating_point_v<T>,
            "h5::exponential requires a floating-point type");

        exponential(T lambda = T{1}) : dist_(lambda) {}
        value_type operator*() { return dist_(impl::rng()); }
    private:
        std::exponential_distribution<T> dist_;
    };

    /** bernoulli{p}  — bool with probability p of true */
    struct bernoulli {
        using value_type = bool;
        bernoulli(double p = 0.5) : dist_(p) {}
        value_type operator*() { return dist_(impl::rng()); }
    private:
        std::bernoulli_distribution dist_;
    };

    /** strlen<CharT>{min_len, max_len}  — random strings over the Latin alphabet */
    template<class CharT = char>
    struct strlen {
        using value_type = std::basic_string<CharT>;
        strlen(size_t min_len = 5, size_t max_len = 20)
            : min_(min_len), max_(max_len) {}
        value_type operator*() {
            return utils::string::random<CharT>(min_, max_);
        }
    private:
        size_t min_, max_;
    };

    /** pod<T>{}  — default-constructed T; specialize for field-level randomness */
    template<class T>
    struct pod {
        using value_type = T;
        static_assert(std::is_default_constructible_v<T>,
            "h5::pod<T> requires T to be default-constructible");
        value_type operator*() const { return T{}; }
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Pretty-print machinery for STL-like containers
//
// Recursively pretty-prints any object that exposes a familiar STL surface to
// std::ostream — vectors, lists, sets, maps, stacks, queues, priority_queues,
// tuples, pairs, ... — without requiring intrusive instrumentation. Each
// inserter is selected by feature detection (Walter Brown's WG21 N4436 idiom):
// which combination of member-functions the type exposes.
//
// Categories:
//   1. iterable (`begin/end`)                            → vector, list, set, map, array, deque
//   2. stack adaptor (`top/pop/empty`, no iterator)      → std::stack, std::priority_queue
//   3. queue adaptor (`front/pop/empty`, no iterator)    → std::queue
//   4. std::pair<K,V>                                    → "{key:value}"
//   5. std::tuple<Ts...>                                 → "<v0,v1,...,vN>"
//
// Long containers truncate at H5CPP_CONSOLE_WIDTH (default 10) with a trailing
// ", ..." so output stays single-line. map<K,V> inherits from the iterable
// case — *it returns pair<const K,V> → pair overload prints "{k:v}", yielding
// "[{k1:v1},{k2:v2},...]".
// ─────────────────────────────────────────────────────────────────────────────

#ifndef H5CPP_CONSOLE_WIDTH
#define H5CPP_CONSOLE_WIDTH 10
#endif

namespace h5::meta {
    // Detectors for adaptor-style containers (no iterators, just top/front + pop + empty).
    template <typename T> using top_f   = decltype(std::declval<T>().top());
    template <typename T> using front_f = decltype(std::declval<T>().front());
    template <typename T> using pop_f   = decltype(std::declval<T&>().pop());
    template <typename T> using empty_f = decltype(std::declval<T>().empty());

    template <typename T> using has_top   = compat::is_detected<top_f, T>;
    template <typename T> using has_front = compat::is_detected<front_f, T>;
    template <typename T> using has_pop   = compat::is_detected<pop_f, T>;
    template <typename T> using has_empty = compat::is_detected<empty_f, T>;

    // Linalg types (Eigen, blaze, arma, ...) expose a `Scalar` nested type for
    // their element. STL containers don't. The pretty-printer uses this as a
    // veto so it doesn't try to iterate types whose begin()/end() static_assert
    // at substitution time (Eigen::Matrix is the canonical offender).
    template <typename T> using scalar_alias_f = typename T::Scalar;
    template <typename T> using has_scalar_alias = compat::is_detected<scalar_alias_f, T>;
}

// Forward declarations: the iterable operator<< below recurses into the
// element type, and for map<K,V> that element is std::pair<const K,V>. With
// two-phase template name lookup, the pair/tuple overloads must be declared
// *before* the iterable template's definition so they're visible at the
// recursive call site. ADL won't reach them — they live in the global
// namespace, while pair/tuple live in std.
template <class K, class V>
inline std::ostream& operator<<(std::ostream& os, const std::pair<K,V>& p);
template <class... Ts>
inline std::ostream& operator<<(std::ostream& os, const std::tuple<Ts...>& t);

// 1. Iterable containers (vector, list, set, map, array, deque, …).
//    Excludes strings (which stream natively), tuples (handled below), and
//    linalg matrix types like Eigen::Matrix (which expose iterator typedefs
//    but static_assert if you call begin() on a non-vector). The `Scalar`
//    nested-type veto identifies linalg types — STL containers use
//    `value_type`, linalg libraries use `Scalar`. is_text_like (not is_string)
//    is used here because is_string is permissive about T's value_type and
//    incorrectly fires for e.g. vector<string>.
template <class T>
inline std::enable_if_t<
    !h5::meta::is_text_like<T>::value &&
    !h5::meta::is_tuple<T>::value &&
    !h5::meta::has_scalar_alias<T>::value &&
    h5::meta::has_iterator<T>::value &&
    h5::meta::has_value_type<T>::value,
std::ostream&>
operator<<(std::ostream& os, const T& container) {
    os << "[";
    auto it   = std::begin(container);
    auto last = std::end(container);
    if (it != last) {
        os << *it;
        std::size_t i = 0;
        while (++it != last && i++ < H5CPP_CONSOLE_WIDTH)
            os << "," << *it;
        if (i >= H5CPP_CONSOLE_WIDTH && it != last) os << ", ...";
    }
    os << "]";
    return os;
}

// 2. Stack-like adaptors (std::stack, std::priority_queue) — destructive copy.
template <class T>
inline std::enable_if_t<
    !h5::meta::is_text_like<T>::value &&
    !h5::meta::has_iterator<T>::value &&
    h5::meta::has_top<T>::value &&
    h5::meta::has_pop<T>::value &&
    h5::meta::has_empty<T>::value,
std::ostream&>
operator<<(std::ostream& os, const T& container_in) {
    T container = container_in;
    os << "[";
    if (!container.empty()) {
        os << container.top(); container.pop();
        std::size_t i = 0;
        while (!container.empty() && i++ < H5CPP_CONSOLE_WIDTH) {
            os << "," << container.top();
            container.pop();
        }
        if (i >= H5CPP_CONSOLE_WIDTH && !container.empty()) os << ", ...";
    }
    os << "]";
    return os;
}

// 3. Queue-like adaptors (std::queue) — destructive copy via front()/pop().
template <class T>
inline std::enable_if_t<
    !h5::meta::is_text_like<T>::value &&
    !h5::meta::has_iterator<T>::value &&
    h5::meta::has_front<T>::value &&
    h5::meta::has_pop<T>::value &&
    h5::meta::has_empty<T>::value,
std::ostream&>
operator<<(std::ostream& os, const T& container_in) {
    T container = container_in;
    os << "[";
    if (!container.empty()) {
        os << container.front(); container.pop();
        std::size_t i = 0;
        while (!container.empty() && i++ < H5CPP_CONSOLE_WIDTH) {
            os << "," << container.front();
            container.pop();
        }
        if (i >= H5CPP_CONSOLE_WIDTH && !container.empty()) os << ", ...";
    }
    os << "]";
    return os;
}

// 4. std::pair<K,V> — feeds into the iterable case for maps too:
//    map<int,string> → [{1:foo},{2:bar},…]
template <class K, class V>
inline std::ostream& operator<<(std::ostream& os, const std::pair<K,V>& p) {
    os << "{" << p.first << ":" << p.second << "}";
    return os;
}

// 5. std::tuple<Ts...> — angle-bracket separated, fold-expression over elements.
template <class... Ts>
inline std::ostream& operator<<(std::ostream& os, const std::tuple<Ts...>& t) {
    os << "<";
    std::apply([&os](auto const&... fields) {
        std::size_t i = 0;
        constexpr std::size_t N = sizeof...(Ts);
        ((os << fields, (++i < N ? (os << "," , 0) : 0)), ...);
    }, t);
    os << ">";
    return os;
}
