#!/bin/bash
# -*- mode: julia -*-
#=
exec julia --color=yes --startup-file=no -e "include(popfirst!(ARGS))" "${BASH_SOURCE[0]}" "$@"
=#

using JLD, SparseArrays

A = sprand(Float64, 10,20, 0.1)
B = sprand(Float64, 10,20, 0.1)
@save "interop.h5" "data-01/A" A "data-02/B" B 
