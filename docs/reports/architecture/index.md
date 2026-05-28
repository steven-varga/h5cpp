@page reports_architecture Architecture

Design documents covering the type-system, threading, async-I/O, scatter/gather, and the h5cpp-compiler's multi-backend architecture. These are the "why does this layer exist and how was it built" docs.

## Members

- @subpage reports_type_system_architecture_notes — how the storage_representation taxonomy + container mappers compose into the unified I/O dispatch
- @subpage reports_type_system_map — visual map of the type-classification taxonomy
- @subpage reports_async_mode_thread_safety — compile-time thread-safety guarantees for async-mode descriptors
- @subpage reports_threaded_pipeline_sigma_queue — the sigma-queue design behind the threaded filter pipeline
- @subpage reports_fapl_multithreading_workplan — the workplan that produced the FAPL-scoped worker pool (v1.12.5 #251)
- @subpage reports_compiler_scatter_gather_design — visitor-style scatter/gather emission from h5cpp-compiler
- @subpage reports_compiler_scatter_gather_visitor_refactor — refactor moving from I/O-heavy scatter to pure conversion functions
- @subpage reports_compiler_multi_backend_architecture — the plug-in serialisation backend framework
- @subpage reports_reflection_cpp26_roadmap — how C++26 static reflection will reshape the compiler
- @subpage reports_performance_evaluation_framework_design — the design behind the performance-comparison harness
- @subpage reports_performance_comparison_framework — comparison framework spec
- @subpage reports_phase_1_3_pt_t_pool_integration_notes — packet-table / pool-allocator integration phase notes
