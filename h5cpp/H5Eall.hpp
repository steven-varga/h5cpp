/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>
#include <stdexcept>
#include <string>
#include <iostream>
namespace h5{
	using herr_t = ::herr_t;

	static thread_local herr_t (*error_stack_callback)(::hid_t, void*);
	static thread_local void * error_stack_client_data;
}

namespace h5 {
	/**  @ingroup file-io
	 * @brief removes default error handler	preventing diagnostic error messages printed
	 * for direct CAPI calls. This is a thread safe implementation. [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * \sa_h5cpp \sa_hdf5 \sa_stl 
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void mute( ){
		H5Eget_auto2(H5E_DEFAULT, &error_stack_callback, &error_stack_client_data);
		H5Eset_auto2(H5E_DEFAULT, nullptr, nullptr);
    }
	/**  @ingroup file-io
	 * @brief restores previously saved error handler with h5::mute [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * @code
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void unmute( ){
		H5Eset_auto2(H5E_DEFAULT, error_stack_callback, error_stack_client_data);
    }
}

namespace h5::error {
	/**
	 * Root exception thrown by every h5cpp call site. Derives from
	 * `std::runtime_error`; a `catch (const std::exception&)` will catch any
	 * h5cpp error.
	 */
	struct any : public std::runtime_error {
		any() : std::runtime_error("H5CPP ERROR") {}
		any(const std::string& msg ) : std::runtime_error( msg ){}
	};
	struct rollback {
		rollback(const std::string& msg ) {
		std::cerr <<"UNRECOVERABLE ERROR:" <<  msg << std::endl;
#ifdef H5CPP_HARD_ERROR
	exit(1)
#endif
		}
	};
	namespace msg {
		const std::string inc_ref   = "couldn't increment reference...";
		const std::string dec_ref   = "couldn't decrement reference...";
		const std::string create_memspace = "couldn't create memory space...";
		const std::string select_memspace = "couldn't do  memory space selection...";
		const std::string select_hyperslab = "couldn't do hyper-slab  selection...";
		const std::string get_filespace = "couldn't get file space...";
		const std::string get_memspace = "couldn't get memory space...";
		const std::string get_memtype = "couldn't get mem type...";
		const std::string get_filetype = "couldn't get file type...";
		const std::string get_filetype_size = "could_t get file type size...";
		const std::string close_memspace = "could_t close memory space...";
		const std::string close_filespace = "could_t close file space...";
		const std::string close_filetype = "could_t close file type...";
		const std::string close_memtype = "could_t close memory type...";
		const std::string close_property_list = "couldn't close property list...";
		const std::string close_group = "couldn't close group/directory...";
		const std::string get_rank = "couldn't obtain rank...";
		const std::string get_chunk_dims = "couldn't obtain chunk dimensions...";
		const std::string get_dims = "couldn't obtain data dimensions...";
		const std::string mem_alloc = "couldn't allocate memory of requested size...";
		const std::string set_extent = "couldn't set extent...";
		const std::string set_chunk = "couldn't set chunk...";
		const std::string write_dataset = "couldn't write dataset...";
		const std::string read_dataset = "couldn't read dataset...";
		const std::string create_group   = "couldn't create group...";
		const std::string create_dataset = "couldn't create dataset...";
		const std::string create_file = "couldn't create file...";
		const std::string create_property_list = "couldn't create property list...";
		const std::string create_dims = "couldn't create dimension descriptor...";
		const std::string link_check = "checking if object exists fails...";
		const std::string open_dataset = "opening dataset failed...";
		const std::string open_file = "opening hdf5 container failed...";
		const std::string open_group = "opening directory failed...";
		const std::string dataset_descriptor = "invalid dataset descriptor ...";
		const std::string file_descriptor = "invalid file descriptor ...";
		const std::string file_space = "invalid file space ...";
		const std::string prop_descriptor = "invalid file descriptor ...";
		const std::string list_directory = "error traversing directory ...";
		const std::string rank_mismatch = "rank of file space and mem space must match ...";
		const std::string get_simple_extent_dims = "???? ...";
		const std::string get_dataset_type = "???? ...";
		const std::string create_dcpl = "???? ...";
	}
}
namespace h5::error::io {
	/**
	 * Rollback signal for I/O code paths. Separate from `runtime_error`.
	 */
	struct rollback : public h5::error::rollback {
		rollback( const std::string& msg ) : h5::error::rollback( msg ){}
	};
	/**
	 * Catches any h5cpp I/O error (file / dataset / packet_table / attribute
	 * / group).
	 */
	struct any : public h5::error::any {
		any() : h5::error::any() {}
		any( const std::string& msg ) : h5::error::any( msg ){}
	};
}

namespace h5::error::io::file {
	/**
	 * Rollback signal for file-level operations.
	 */
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	/**
	 * Catches any file-level I/O error.
	 */
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	/**
	 * Thrown when `h5::open(file_path, ...)` fails — file does not exist,
	 * permission denied, HDF5 magic mismatch, or unsupported on-disk format.
	 */
	struct open : public h5::error::io::file::any {
		open() : h5::error::io::file::any() {}
		open( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	/**
	 * Thrown when `h5::fd_t` destruction or explicit `H5Fclose` fails (rare;
	 * typically a pending operation or resource leak).
	 */
	struct close : public h5::error::io::file::any {
		close() : h5::error::io::file::any() {}
		close( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	/**
	 * Thrown by file-level read paths (metadata block scan, format-version
	 * probe).
	 */
	struct read : public h5::error::io::file::any {
		read() : h5::error::io::file::any() {}
		read( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	/**
	 * Thrown by file-level write paths (file flush, metadata commit).
	 */
	struct write : public h5::error::io::file::any {
		write() : h5::error::io::file::any() {}
		write( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	/**
	 * Thrown when `h5::create(file_path, ...)` fails — path not writable,
	 * `H5F_ACC_EXCL` conflict, parent directory missing, FAPL invalid.
	 */
	struct create : public h5::error::io::file::any {
		create() : h5::error::io::file::any() {}
		create( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous file-level failures (mount, refresh,
	 * get_info).
	 */
	struct misc : public h5::error::io::file::any {
		misc() : h5::error::io::file::any() {}
		misc( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
}

namespace h5::error::io::dataset {
	/**
	 * Rollback signal for dataset-level operations.
	 */
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	/**
	 * Catches any dataset-level I/O error.
	 */
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	/**
	 * Thrown when `h5::open(fd, path)` fails to locate or open the dataset.
	 */
	struct open : public h5::error::io::dataset::any {
		open() : h5::error::io::dataset::any() {}
		open( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown when `h5::ds_t` destruction or explicit `H5Dclose` fails
	 * (rare).
	 */
	struct close : public h5::error::io::dataset::any {
		close() : h5::error::io::dataset::any() {}
		close( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown when `h5::read` fails — selection out of bounds,
	 * type-conversion error, chunked-read or filter error.
	 */
	struct read : public h5::error::io::dataset::any {
		read() : h5::error::io::dataset::any() {}
		read( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown when `h5::write` fails — selection mismatch, extending error,
	 * filter / compression error, ROS3 read-only attempt.
	 */
	struct write : public h5::error::io::dataset::any {
		write() : h5::error::io::dataset::any() {}
		write( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown when `h5::append` fails on an extensible dataset (e.g. missing
	 * chunk layout).
	 */
	struct append : public h5::error::io::dataset::any {
		append() : h5::error::io::dataset::any() {}
		append( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown when `h5::create<T>(...)` fails — parent path missing, chunk
	 * dims invalid, DCPL conflict.
	 */
	struct create : public h5::error::io::dataset::any {
		create() : h5::error::io::dataset::any() {}
		create( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous dataset failures (set_extent, refresh,
	 * get_storage_size).
	 */
	struct misc : public h5::error::io::dataset::any {
		misc() : h5::error::io::dataset::any() {}
		misc( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
}
namespace h5::error::io::packet_table {
	/**
	 * Rollback signal for packet-table operations.
	 */
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	/**
	 * Catches any packet-table I/O error (extensible-dataset append
	 * interface).
	 */
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	/**
	 * Thrown when `h5::pt_t` open fails.
	 */
	struct open : public h5::error::io::packet_table::any {
		open() : h5::error::io::packet_table::any() {}
		open( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown when `h5::pt_t` destruction fails.
	 */
	struct close : public h5::error::io::packet_table::any {
		close() : h5::error::io::packet_table::any() {}
		close( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown when a packet-table read fails.
	 */
	struct read : public h5::error::io::packet_table::any {
		read() : h5::error::io::packet_table::any() {}
		read( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown when a packet-table write fails.
	 */
	struct write : public h5::error::io::packet_table::any {
		write() : h5::error::io::packet_table::any() {}
		write( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown when `h5::append(pt, …)` fails — buffer overflow, extending
	 * error, or filter error on the underlying chunked dataset.
	 */
	struct append : public h5::error::io::packet_table::any {
		append() : h5::error::io::packet_table::any() {}
		append( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown when packet-table creation fails — missing chunk layout,
	 * invalid DCPL.
	 */
	struct create : public h5::error::io::packet_table::any {
		create() : h5::error::io::packet_table::any() {}
		create( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous packet-table failures.
	 */
	struct misc : public h5::error::io::packet_table::any {
		misc() : h5::error::io::packet_table::any() {}
		misc( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
}

namespace h5::error::io::attribute {
	/**
	 * Rollback signal for attribute-level operations.
	 */
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	/**
	 * Catches any attribute-level I/O error.
	 */
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	/**
	 * Thrown when `h5::aopen` (or attribute-bracket access `ds["name"]`)
	 * fails to find or open the attribute.
	 */
	struct open : public h5::error::io::attribute::any {
		open() : h5::error::io::attribute::any() {}
		open( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown when `h5::at_t` destruction fails (rare).
	 */
	struct close : public h5::error::io::attribute::any {
		close() : h5::error::io::attribute::any() {}
		close( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown when `h5::aread<T>` fails — type-mismatch, missing fixed↔VLEN
	 * conversion, or selection error.
	 */
	struct read : public h5::error::io::attribute::any {
		read() : h5::error::io::attribute::any() {}
		read( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown when `h5::awrite` fails — type-mismatch, attribute already
	 * present with incompatible shape, or invalid parent.
	 */
	struct write : public h5::error::io::attribute::any {
		write() : h5::error::io::attribute::any() {}
		write( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown when `h5::acreate` fails — duplicate name, invalid type,
	 * invalid parent.
	 */
	struct create : public h5::error::io::attribute::any {
		create() : h5::error::io::attribute::any() {}
		create( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous attribute failures (rename, get_info,
	 * iterate).
	 */
	struct misc : public h5::error::io::attribute::any {
		misc() : h5::error::io::attribute::any() {}
		misc( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	/**
	 * Thrown when `h5::adelete` (or `ds.attr_delete("name")`) fails —
	 * attribute missing, parent immutable.
	 */
	struct delete_ : public h5::error::io::attribute::any {
		delete_() : h5::error::io::attribute::any() {}
		delete_( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
}

namespace h5::error::io::group {
	/**
	 * Catches any group-level I/O error.
	 */
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	/**
	 * Thrown when `h5::gopen` fails — group path missing, parent not a
	 * group, permission denied.
	 */
	struct open : public h5::error::io::group::any {
		open() : h5::error::io::group::any() {}
		open( const std::string& msg ) : h5::error::io::group::any( msg ){}
	};
	/**
	 * Thrown when `h5::gr_t` destruction or explicit `H5Gclose` fails
	 * (rare).
	 */
	struct close : public h5::error::io::group::any {
		close() : h5::error::io::group::any() {}
		close( const std::string& msg ) : h5::error::io::group::any( msg ){}
	};
	/**
	 * Thrown when `h5::gcreate` fails — parent path missing (without
	 * `create_intermediates`), duplicate name, invalid LCPL.
	 */
	struct create : public h5::error::io::group::any {
		create() : h5::error::io::group::any() {}
		create( const std::string& msg ) : h5::error::io::group::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous group failures (get_info, link iteration).
	 */
	struct misc : public h5::error::io::group::any {
		misc() : h5::error::io::group::any() {}
		misc( const std::string& msg ) : h5::error::io::group::any( msg ){}
	};
}

namespace h5::error::property_list {
	/**
	 * Rollback signal for property-list construction.
	 */
	struct rollback : public h5::error::rollback {
		rollback( const std::string& msg ) : h5::error::rollback( msg ){}
	};
	/**
	 * Catches any property-list error (FAPL / FCPL / DAPL / DCPL / DXPL /
	 * LCPL).
	 */
	struct any : public h5::error::any {
		any() : h5::error::any() {}
		any( const std::string& msg ) : h5::error::any( msg ){}
	};
	/**
	 * Thrown when a property-list argument is invalid — wrong type,
	 * out-of-range value, missing required field, or conflicting flags.
	 */
	struct argument : public h5::error::property_list::any {
		argument() : h5::error::property_list::any() {}
		argument( const std::string& msg ) : h5::error::property_list::any( msg ){}
	};
	/**
	 * Thrown for miscellaneous property-list failures (copy, equal, close).
	 */
	struct misc : public h5::error::property_list::any {
		misc() : h5::error::property_list::any() {}
		misc( const std::string& msg ) : h5::error::property_list::any( msg ){}
	};
}
