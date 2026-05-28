@page reports_compatibility Compatibility & quality

Documents covering how h5cpp adapts to HDF5 across versions, the bugs it works around, and the falsifications that shape its struct support.

## Members

- @subpage reports_compatibility_mechanisms — the version-gating and conversion-policy mechanisms that keep h5cpp portable across HDF5 1.10 / 1.12 / 1.14
- @subpage reports_study_hdf5_bugs_top10 — the ten HDF5 bugs / quirks that have most shaped h5cpp's defensive code
- @subpage reports_doxygen_alias_support_report — historical assessment of Doxygen alias support (this is what motivated the alias system you see today)
- @subpage reports_study_struct_falsification — what struct shapes h5cpp can and cannot support, with concrete counter-examples
