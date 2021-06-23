# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# IFC DOM Library

add_library(ifc-dom STATIC 
	charts.cxx
	decls.cxx
	exprs.cxx
	literals.cxx
	names.cxx
	sentences.cxx
	stmts.cxx
	syntax.cxx
	types.cxx)

target_link_libraries(ifc-dom PRIVATE GSL)
