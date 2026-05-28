/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
 *
 * Smart-pointer write/read forwarding overloads. Companion to H5Mmemory.hpp
 * (the traits half).  Pulled in from h5cpp/io after H5Dwrite/H5Dread so the
 * raw-pointer overloads we delegate to are already visible.
 */
#pragma once

#include <memory>

namespace h5 {
    // ── write: smart_ptr<T[]> -> raw T* ─────────────────────────────────────
    template <class T, class... args_t>
    inline void write(const h5::fd_t& fd, const std::string& path,
                      const std::unique_ptr<T[]>& p, args_t&&... args) {
        h5::write<T>(fd, path, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void write(const h5::fd_t& fd, const std::string& path,
                      const std::shared_ptr<T[]>& p, args_t&&... args) {
        h5::write<T>(fd, path, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void write(const h5::ds_t& ds,
                      const std::unique_ptr<T[]>& p, args_t&&... args) {
        h5::write<T>(ds, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void write(const h5::ds_t& ds,
                      const std::shared_ptr<T[]>& p, args_t&&... args) {
        h5::write<T>(ds, p.get(), std::forward<args_t>(args)...);
    }

    // ── write: single-T smart_ptr (1-element dataset) ──────────────────────
    template <class T, class... args_t>
    inline void write(const h5::fd_t& fd, const std::string& path,
                      const std::unique_ptr<T>& p, args_t&&... args) {
        h5::write<T>(fd, path, p.get(), h5::count{1}, std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void write(const h5::fd_t& fd, const std::string& path,
                      const std::shared_ptr<T>& p, args_t&&... args) {
        h5::write<T>(fd, path, p.get(), h5::count{1}, std::forward<args_t>(args)...);
    }

    // ── read: smart_ptr<T[]> -> raw T* (caller pre-allocates) ──────────────
    template <class T, class... args_t>
    inline void read(const h5::fd_t& fd, const std::string& path,
                     std::unique_ptr<T[]>& p, args_t&&... args) {
        h5::read<T>(fd, path, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void read(const h5::fd_t& fd, const std::string& path,
                     std::shared_ptr<T[]>& p, args_t&&... args) {
        h5::read<T>(fd, path, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void read(const h5::ds_t& ds,
                     std::unique_ptr<T[]>& p, args_t&&... args) {
        h5::read<T>(ds, p.get(), std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void read(const h5::ds_t& ds,
                     std::shared_ptr<T[]>& p, args_t&&... args) {
        h5::read<T>(ds, p.get(), std::forward<args_t>(args)...);
    }

    // ── read: single-T smart_ptr ───────────────────────────────────────────
    template <class T, class... args_t>
    inline void read(const h5::fd_t& fd, const std::string& path,
                     std::unique_ptr<T>& p, args_t&&... args) {
        h5::read<T>(fd, path, p.get(), h5::count{1}, std::forward<args_t>(args)...);
    }
    template <class T, class... args_t>
    inline void read(const h5::fd_t& fd, const std::string& path,
                     std::shared_ptr<T>& p, args_t&&... args) {
        h5::read<T>(fd, path, p.get(), h5::count{1}, std::forward<args_t>(args)...);
    }
}
