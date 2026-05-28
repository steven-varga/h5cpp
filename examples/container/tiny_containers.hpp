// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// Minimal, header-only containers that satisfy the *contract* h5cpp's
// detection-idiom dispatch looks for. None of them inherits from std:: or
// reuses any std::vector/std::list/std::map internals — they exist purely to
// show the three structural shapes the layer-2 fallback maps onto:
//
//   1. tiny::vec<T>   — contiguous sequence  (.data() + .size() + value_type)
//                        → access_traits_t kind=contiguous, linear_value_dataset
//   2. tiny::flist<T> — iterator-only sequence (begin/end + value_type, no data)
//                        → access_traits_t kind=iterators,  linear_value_dataset
//   3. tiny::dict<K,V>— map-like (key_type + mapped_type + value_type)
//                        → access_traits_t kind=iterators,  key_value_dataset
//
// They are intentionally simple: no allocator template, no move-only games, no
// debug guards. The point is the *trait surface* the dispatcher sees, not the
// container quality.
#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <utility>

namespace tiny {

    // ── 1. contiguous vector-shape ──────────────────────────────────────────────
    // Trait surface picked up by h5cpp (all via Walter Brown detection):
    //   value_type, iterator, const_iterator,           ← nested aliases
    //   begin/end, size, data                           ← member expressions
    // Storage rep resolves via the *sequential_like* fallback in H5Tmeta.hpp:321.
    template <class T>
    struct vec {
        using value_type      = T;
        using iterator        = T*;
        using const_iterator  = const T*;

        std::unique_ptr<T[]> buf_;
        std::size_t          n_ = 0;

        vec() = default;
        vec(std::initializer_list<T> il) : buf_(new T[il.size()]), n_(il.size()) {
            std::size_t i = 0; for (const auto& v : il) buf_[i++] = v;
        }
        explicit vec(std::size_t n) : buf_(new T[n]), n_(n) {}

        T*       data()       noexcept { return buf_.get(); }
        const T* data() const noexcept { return buf_.get(); }
        std::size_t size() const noexcept { return n_; }

        T*       begin()       noexcept { return buf_.get(); }
        T*       end()         noexcept { return buf_.get() + n_; }
        const T* begin() const noexcept { return buf_.get(); }
        const T* end()   const noexcept { return buf_.get() + n_; }
    };

    // ── 2. iterator-only sequence-shape (singly linked) ─────────────────────────
    // Surface: value_type, iterator, begin/end, size — but *no* data() member.
    // h5cpp's access_traits_t catches this via the iterator-only spec at
    // H5Tmeta.hpp:850 and routes the write through the staging-vector path.
    template <class T>
    class flist {
        struct node { T value; node* next; };
        node*       head_ = nullptr;
        node*       tail_ = nullptr;
        std::size_t n_    = 0;
    public:
        using value_type = T;

        struct iterator {
            node* p;
            using value_type = T; using difference_type = std::ptrdiff_t;
            using pointer = T*; using reference = T&;
            using iterator_category = std::forward_iterator_tag;
            T& operator*() const { return p->value; }
            iterator& operator++() { p = p->next; return *this; }
            iterator  operator++(int) { auto t = *this; p = p->next; return t; }
            bool operator==(const iterator& o) const { return p == o.p; }
            bool operator!=(const iterator& o) const { return p != o.p; }
        };
        using const_iterator = iterator;

        flist() = default;
        flist(std::initializer_list<T> il) { for (const auto& v : il) push_back(v); }
        ~flist() { while (head_) { node* n = head_->next; delete head_; head_ = n; } }
        flist(const flist&) = delete;            // keep example minimal
        flist& operator=(const flist&) = delete;

        void push_back(const T& v) {
            node* n = new node{v, nullptr};
            if (!head_) head_ = n; else tail_->next = n;
            tail_ = n; ++n_;
        }

        std::size_t size() const noexcept { return n_; }
        iterator begin() const noexcept { return {head_}; }
        iterator end()   const noexcept { return {nullptr}; }
    };

    // ── 3. set-shape (key_type, no mapped_type) ─────────────────────────────────
    // Surface: key_type, value_type = K, key_compare, iterator, begin/end, size.
    // No mapped_type — that's what tells h5cpp this is a set, not a map.
    // Backed by a sorted std::unique_ptr<K[]> arena rebuilt on insert (toy O(n²)).
    template <class K, class Compare = std::less<K>>
    class set {
    public:
        using key_type     = K;
        using value_type   = K;
        using key_compare  = Compare;
        using iterator       = const K*;
        using const_iterator = const K*;

        set() = default;
        set(std::initializer_list<K> il) { for (const auto& k : il) insert(k); }

        void insert(const K& k) {
            Compare cmp{};
            std::size_t i = 0;
            while (i < n_ && cmp(buf_[i], k)) ++i;
            if (i < n_ && !cmp(k, buf_[i])) return;          // duplicate
            auto fresh = std::unique_ptr<K[]>(new K[n_ + 1]);
            for (std::size_t j = 0; j < i; ++j) fresh[j] = buf_[j];
            fresh[i] = k;
            for (std::size_t j = i; j < n_; ++j) fresh[j + 1] = buf_[j];
            buf_ = std::move(fresh); ++n_;
        }

        std::size_t size() const noexcept { return n_; }
        const_iterator begin() const noexcept { return buf_.get(); }
        const_iterator end()   const noexcept { return buf_.get() + n_; }

    private:
        std::unique_ptr<K[]> buf_;
        std::size_t          n_ = 0;
    };

    // ── 4. map-shape (key → mapped_type) ────────────────────────────────────────
    // Surface: key_type, mapped_type, value_type=pair<const K,V>, key_compare,
    // iterator, begin/end, size. h5cpp catches this via is_map_like at
    // H5Tmeta.hpp:329 and routes through the kv_t compound write path.
    //
    // Backed by a sorted std::unique_ptr<node[]> arena rebuilt on insert — toy
    // implementation, intentionally O(n²) at fill time.
    template <class K, class V, class Compare = std::less<K>>
    class dict {
    public:
        using key_type     = K;
        using mapped_type  = V;
        using value_type   = std::pair<const K, V>;
        using key_compare  = Compare;
        using iterator       = value_type*;
        using const_iterator = const value_type*;

        dict() = default;
        dict(std::initializer_list<std::pair<K,V>> il) { for (const auto& kv : il) emplace(kv.first, kv.second); }

        void emplace(const K& k, const V& v) {
            auto fresh = std::unique_ptr<value_type[]>(new value_type[n_ + 1]);
            std::size_t i = 0;
            Compare cmp{};
            while (i < n_ && cmp(buf_[i].first, k)) {
                new (&fresh[i]) value_type(buf_[i].first, buf_[i].second); ++i;
            }
            new (&fresh[i]) value_type(k, v);
            for (std::size_t j = i; j < n_; ++j)
                new (&fresh[j + 1]) value_type(buf_[j].first, buf_[j].second);
            buf_ = std::move(fresh); ++n_;
        }

        std::size_t size() const noexcept { return n_; }
        iterator       begin()       noexcept { return buf_.get(); }
        iterator       end()         noexcept { return buf_.get() + n_; }
        const_iterator begin() const noexcept { return buf_.get(); }
        const_iterator end()   const noexcept { return buf_.get() + n_; }

    private:
        std::unique_ptr<value_type[]> buf_;
        std::size_t                   n_ = 0;
    };

} // namespace tiny
