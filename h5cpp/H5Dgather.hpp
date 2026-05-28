/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once

#include <cstddef>
#include <vector>
#include <string>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <string.h>

namespace h5 {
    namespace detail {
        template <class...> struct dependent_false_t : std::false_type {};
    }

    template <class container_t, class T> inline
    const T* gather_linear_values(const container_t& ref, std::vector<T>& elements) {
        elements.clear();
        elements.insert(elements.end(), ref.begin(), ref.end());
        return elements.data();
    }

    template <class T, class A> inline
    const T* gather(const std::list<T,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class A> inline
    const T* gather(const std::forward_list<T,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class A> inline
    const T* gather(const std::deque<T,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class C, class A> inline
    const T* gather(const std::set<T,C,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class C, class A> inline
    const T* gather(const std::multiset<T,C,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class H, class E, class A> inline
    const T* gather(const std::unordered_set<T,H,E,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class H, class E, class A> inline
    const T* gather(const std::unordered_multiset<T,H,E,A>& ref, std::vector<T>& elements){
        return gather_linear_values(ref, elements);
    }
    template <class T, class A> inline
    const T* gather(const std::vector<T,A>& ref, std::vector<T>& elements){
        return ref.data();
    }

    template <class T, class A> inline
    void materialize(std::list<T,A>& ref, const T* first, const T* last){
        ref.assign(first, last);
    }
    template <class T, class A> inline
    void materialize(std::list<T,A>& ref, const T* ptr, size_t size){
        materialize(ref, ptr, ptr + size);
    }
    template <class T, class A> inline
    void materialize(std::forward_list<T,A>& ref, const T* first, const T* last){
        ref.assign(first, last);
    }
    template <class T, class A> inline
    void materialize(std::forward_list<T,A>& ref, const T* ptr, size_t size){
        materialize(ref, ptr, ptr + size);
    }
    template <class T, class A> inline
    void materialize(std::deque<T,A>& ref, const T* first, const T* last){
        ref.assign(first, last);
    }
    template <class T, class A> inline
    void materialize(std::deque<T,A>& ref, const T* ptr, size_t size){
        materialize(ref, ptr, ptr + size);
    }
    template <class T, class C, class A> inline
    void materialize(std::set<T,C,A>& ref, const T* first, const T* last){
        ref.clear();
        ref.insert(first, last);
    }
    template <class T, class C, class A> inline
    void materialize(std::set<T,C,A>& ref, const T* ptr, size_t size){
        materialize(ref, ptr, ptr + size);
    }
    template <class T, class H, class E, class A> inline
    void materialize(std::unordered_set<T,H,E,A>& ref, const T* first, const T* last){
        ref.clear();
        ref.insert(first, last);
    }
    template <class T, class H, class E, class A> inline
    void materialize(std::unordered_set<T,H,E,A>& ref, const T* ptr, size_t size){
        materialize(ref, ptr, ptr + size);
    }

    /** @brief gathers memory regions into a single set of pointers
     * Objects may be classified by whether the content resides in contiguous memory location, making convenient
     * to IO operation: only single call is needed; or scattered and we need a reliable mechanism to collect the 
     * content. <br/>
     * `h5::gather` is a template mechanism to facilitate the latter process by finding and returning a set of 
     * `element_t` type pointers to the actual content of an object.  
     * \par_ref \par_ptr
     * @return ptr the same `element_t` pointer passed to the call (the in-out parameter is returned unchanged for chaining)
     * \tpar_T
     */ 
    inline const char** gather( const std::vector<std::string>& ref, std::vector<const char*>& ptrs){
        ptrs.clear();
        for(auto& element: ref)
           ptrs.push_back(element.data());
        return ptrs.data();
    } 

    template <class T, class E> inline
    const E* gather(const T&, std::vector<E>&){
#ifndef _MSC_VER
        static_assert(detail::dependent_false_t<T,E>::value,
            "h5::gather path is unsupported for this type");
#endif
        return nullptr;
    }

    template <class T, class E> inline
    void materialize(T&, const E*, const E*){
#ifndef _MSC_VER
        static_assert(detail::dependent_false_t<T,E>::value,
            "h5::materialize path is unsupported for this type");
#endif
    }

    template <class T, class E> inline
    void materialize(T&, const E*, size_t){
#ifndef _MSC_VER
        static_assert(detail::dependent_false_t<T,E>::value,
            "h5::materialize path is unsupported for this type");
#endif
    }
}
