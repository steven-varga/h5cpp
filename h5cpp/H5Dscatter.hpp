/*
 * Copyright (c) 2018 - 2026 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>
#include <string>
#include <type_traits>

namespace h5 {

/**
 * @brief Compile-time trait to determine if a type uses scatter/gather I/O.
 *
 * The h5cpp-compiler emits specializations of h5::scatter<T> and h5::gather<T>
 * for user-defined structs with heap-indirection fields (std::vector, std::string,
 * etc.). The H5CPP_REGISTER_SCATTER(T) macro sets this trait to true_type.
 *
 * @tparam T C++ type being queried
 */
template <typename T> struct has_scatter : std::false_type {};

/**
 * @brief Registers a type as scatter/gather eligible.
 *
 * Place this macro at file scope next to the compiler-generated scatter/gather
 * specializations. It sets h5::has_scatter<T> to std::true_type, enabling
 * dispatch in h5::write and h5::read.
 */
#define H5CPP_REGISTER_SCATTER( SCATTER_TYPE ) \
    template<> struct h5::has_scatter<SCATTER_TYPE> : std::true_type {}

namespace detail {
    template <class...> struct dependent_false_t : std::false_type {};

    /**
     * @brief Returns the next row index for a 1-D extendable dataset.
     *
     * Queries the current extent of the dataset and returns dims[0],
     * which is the index of the next append position.
     *
     * @param dset valid HDF5 dataset identifier
     * @return hsize_t next row index
     */
    inline hsize_t next_row(hid_t dset) {
        hid_t file_space = H5Dget_space(dset);
        hsize_t dims[1] = {0};
        H5Sget_simple_extent_dims(file_space, dims, nullptr);
        H5Sclose(file_space);
        return dims[0];
    }

    /**
     * @brief Writes a single row to a 1-D dataset, extending it by one element.
     *
     * Grows the dataset to row + 1, selects a single-element hyperslab at
     * the given row, and writes the buffer.
     *
     * @param dset valid HDF5 dataset identifier
     * @param ctype HDF5 compound type describing the row layout
     * @param row row index to write (dataset is extended if necessary)
     * @param buf pointer to row data
     * @return herr_t HDF5 error code (negative on failure)
     */
    inline herr_t write_one_row(hid_t dset, hid_t ctype, hsize_t row, const void* buf) {
        hsize_t new_extent = row + 1;
        herr_t err = H5Dset_extent(dset, &new_extent);
        if (err < 0) return err;

        hid_t file_space = H5Dget_space(dset);
        hsize_t one = 1;
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, &row, nullptr, &one, nullptr);

        hid_t mem_space = H5Screate_simple(1, &one, nullptr);
        err = H5Dwrite(dset, ctype, mem_space, file_space, H5P_DEFAULT, buf);

        H5Sclose(mem_space);
        H5Sclose(file_space);
        return err;
    }

    /**
     * @brief Reads a single row from a 1-D dataset.
     *
     * Selects a single-element hyperslab at the given row and reads into buf.
     *
     * @param dset valid HDF5 dataset identifier
     * @param ctype HDF5 compound type describing the row layout
     * @param row row index to read
     * @param buf pointer to destination buffer
     * @return herr_t HDF5 error code (negative on failure)
     */
    inline herr_t read_one_row(hid_t dset, hid_t ctype, hsize_t row, void* buf) {
        hid_t file_space = H5Dget_space(dset);
        hsize_t one = 1;
        H5Sselect_hyperslab(file_space, H5S_SELECT_SET, &row, nullptr, &one, nullptr);

        hid_t mem_space = H5Screate_simple(1, &one, nullptr);
        herr_t err = H5Dread(dset, ctype, mem_space, file_space, H5P_DEFAULT, buf);

        H5Sclose(mem_space);
        H5Sclose(file_space);
        return err;
    }
} // namespace detail

/**
 * @brief Generic scatter (write) template for tier-2+ types.
 *
 * The primary template has no definition. h5cpp-compiler emits specializations
 * that open/create the dataset, build the row_t mirror, and call
 * h5::detail::write_one_row.
 *
 * @tparam T user-defined struct type with heap-indirection fields
 * @return h5::ds_t the dataset handle (for chaining)
 */
template <typename T>
inline h5::ds_t scatter(hid_t fd, const std::string& path, const T& obj);

/**
 * @brief Generic gather (read) template for tier-2+ types.
 *
 * The primary template has no definition. h5cpp-compiler emits specializations
 * that open the dataset, read a row via h5::detail::read_one_row, and copy
 * VLEN data into the user's containers.
 *
 * @tparam T user-defined struct type with heap-indirection fields
 */
template <typename T>
inline void gather(hid_t fd, const std::string& path, T& obj);

} // namespace h5
