@page curated_io_api IO API

@brief Curated reference for the H5CPP I/O surface — files, datasets,
attributes, and groups. One page per object kind, hand-written for
readability rather than auto-generated from headers.

The four sub-pages below cover the public free-function surface for
each HDF5 object kind. Element type `T` follows the
[Supported Types](@ref link_base_template_types) dispatch matrix
across all four; lifetime is RAII via the
[handle family](@ref link_handle_reference); errors surface through
the [error hierarchy](@ref link_error_handler).

- @subpage curated_io_api_file
- @subpage curated_io_api_dataset
- @subpage curated_io_api_attributes
- @subpage curated_io_api_groups
