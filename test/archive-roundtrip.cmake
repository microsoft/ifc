# Copyright Microsoft Corporation.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# End-to-end round-trip test for the `ifc archive` and `ifc extract` subcommands.
# Invoked as:
#   cmake -DIFC_TOOL=<ifc executable> -DWORK=<dir containing m.ifc and n.ifc> -P archive-roundtrip.cmake
# Only the ifc tool and cmake are run here, so no MSVC environment is required at test time.
# The first unmet expectation aborts with a non-zero exit via message(FATAL_ERROR).

if(NOT IFC_TOOL OR NOT WORK)
  message(FATAL_ERROR "archive-roundtrip.cmake requires -DIFC_TOOL and -DWORK")
endif()

set(fixture "${CMAKE_CURRENT_LIST_DIR}/archive-fixture.ps1")

# -- A separate working directory is needed to exercise relative-path normalization faithfully.
function(run_ok_in directory)
  execute_process(COMMAND ${ARGN} WORKING_DIRECTORY "${directory}" RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "expected success but got exit ${rc}: ${ARGN}")
  endif()
endfunction()

# -- Most commands run in the fixture directory; keep that policy out of individual test cases.
function(run_ok)
  run_ok_in("${WORK}" ${ARGN})
endfunction()

# -- Rejected relative-path cases use their own working directory without weakening diagnostics.
function(run_fail_in directory)
  execute_process(COMMAND ${ARGN} WORKING_DIRECTORY "${directory}" RESULT_VARIABLE rc
                  OUTPUT_QUIET ERROR_QUIET)
  if(rc EQUAL 0)
    message(FATAL_ERROR "expected failure but the command succeeded: ${ARGN}")
  endif()
endfunction()

# -- Most rejected commands run in the fixture directory and suppress their expected diagnostics.
function(run_fail)
  run_fail_in("${WORK}" ${ARGN})
endfunction()

# -- Binary fixture mutations share one checked PowerShell invocation contract.
function(make_fixture mode input output)
  run_ok(powershell -NoProfile -ExecutionPolicy Bypass -File "${fixture}"
         -Mode "${mode}" -InputFile "${input}" -OutputFile "${output}")
endfunction()

# Start from a clean slate, keeping the build-produced member IFCs (m.ifc, n.ifc).
file(REMOVE_RECURSE
  "${WORK}/out" "${WORK}/named" "${WORK}/pkg.ifc"
  "${WORK}/nested.ifc" "${WORK}/coll.ifc" "${WORK}/mcopy.ifc"
  "${WORK}/quote.ifc" "${WORK}/angle.ifc" "${WORK}/sorts.ifc" "${WORK}/sorts-out"
  "${WORK}/module-only" "${WORK}/quote-only" "${WORK}/angle-only"
  "${WORK}/tampered-content.ifc" "${WORK}/tampered-hash.ifc"
  "${WORK}/unsorted.ifc" "${WORK}/bad-path.ifc" "${WORK}/overlap.ifc" "${WORK}/invalid-out"
  "${WORK}/path-parent" "${WORK}/path-work" "${WORK}/path.ifc"
  "${WORK}/junction-source" "${WORK}/junction-out" "${WORK}/junction-outside"
  "${WORK}/junction.ifc")

# Archive two members, keyed by UnitSort plus canonical name, recording an archive name.
run_ok("${IFC_TOOL}" archive -o pkg.ifc --name testpkg m.ifc n.ifc)

# `version` accepts the archive.
run_ok("${IFC_TOOL}" version pkg.ifc)

# Extract everything and confirm each member is byte-identical to its original.
run_ok("${IFC_TOOL}" extract -o out pkg.ifc --all)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/m.ifc" "${WORK}/out/m.ifc")
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/n.ifc" "${WORK}/out/n.ifc")

# Extract a single member by canonical name; the other member must not be written.
run_ok("${IFC_TOOL}" extract -o named pkg.ifc m)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/m.ifc" "${WORK}/named/m.ifc")
if(EXISTS "${WORK}/named/n.ifc")
  message(FATAL_ERROR "selective extract wrote an unrequested member")
endif()

# Existing destinations are preserved unless replacement authority is explicit.
file(WRITE "${WORK}/named/m.ifc" "sentinel")
run_fail("${IFC_TOOL}" extract -o named pkg.ifc m)
file(READ "${WORK}/named/m.ifc" preserved)
if(NOT preserved STREQUAL "sentinel")
  message(FATAL_ERROR "extract modified an existing file without --force")
endif()
run_ok("${IFC_TOOL}" extract --force -o named pkg.ifc m)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/m.ifc" "${WORK}/named/m.ifc")

# Negative cases: unknown name, nesting an archive, and duplicate member identities.
run_fail("${IFC_TOOL}" extract -o bad pkg.ifc nosuchname)
run_fail("${IFC_TOOL}" archive -o nested.ifc pkg.ifc)
run_ok("${CMAKE_COMMAND}" -E copy m.ifc mcopy.ifc)
run_fail("${IFC_TOOL}" archive -o coll.ifc m.ifc mcopy.ifc)

# Header-unit delimiter form is part of its canonical name and must survive CLI selection.
make_fixture(quote-header-unit "${WORK}/m.ifc" "${WORK}/quote.ifc")
make_fixture(angle-header-unit "${WORK}/m.ifc" "${WORK}/angle.ifc")
run_ok("${IFC_TOOL}" archive -o sorts.ifc m.ifc quote.ifc angle.ifc)
run_ok("${IFC_TOOL}" extract -o sorts-out sorts.ifc --all)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/m.ifc" "${WORK}/sorts-out/m.ifc")
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/quote.ifc" "${WORK}/sorts-out/quote.ifc")
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/angle.ifc" "${WORK}/sorts-out/angle.ifc")

run_ok("${IFC_TOOL}" extract -o module-only sorts.ifc m)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/m.ifc" "${WORK}/module-only/m.ifc")
if(EXISTS "${WORK}/module-only/quote.ifc" OR EXISTS "${WORK}/module-only/angle.ifc")
  message(FATAL_ERROR "module selector extracted a header unit")
endif()

run_ok("${IFC_TOOL}" extract -o quote-only sorts.ifc --quote-header m)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/quote.ifc" "${WORK}/quote-only/quote.ifc")
if(EXISTS "${WORK}/quote-only/m.ifc" OR EXISTS "${WORK}/quote-only/angle.ifc")
  message(FATAL_ERROR "quote-header selector extracted another member form")
endif()

run_ok("${IFC_TOOL}" extract -o angle-only sorts.ifc --angle-header=m)
run_ok("${CMAKE_COMMAND}" -E compare_files "${WORK}/angle.ifc" "${WORK}/angle-only/angle.ifc")
if(EXISTS "${WORK}/angle-only/m.ifc" OR EXISTS "${WORK}/angle-only/quote.ifc")
  message(FATAL_ERROR "angle-header selector extracted another member form")
endif()

run_fail("${IFC_TOOL}" extract -o bad sorts.ifc --quote-header absent)
run_fail("${IFC_TOOL}" extract -o bad sorts.ifc --all --angle-header m)
run_fail("${IFC_TOOL}" extract -o bad sorts.ifc --quote-header)
run_fail("${IFC_TOOL}" extract -o bad sorts.ifc "\"m\"")
run_fail("${IFC_TOOL}" extract -o bad sorts.ifc "<m>")

# Integrity failures and authenticated-but-malformed metadata are distinct rejection paths.
make_fixture(tamper-content "${WORK}/pkg.ifc" "${WORK}/tampered-content.ifc")
make_fixture(tamper-hash "${WORK}/pkg.ifc" "${WORK}/tampered-hash.ifc")
make_fixture(unsorted-toc "${WORK}/pkg.ifc" "${WORK}/unsorted.ifc")
make_fixture(bad-path-offset "${WORK}/pkg.ifc" "${WORK}/bad-path.ifc")
make_fixture(overlap-extents "${WORK}/pkg.ifc" "${WORK}/overlap.ifc")
run_fail("${IFC_TOOL}" extract -o invalid-out tampered-content.ifc --all)
run_fail("${IFC_TOOL}" extract -o invalid-out tampered-hash.ifc --all)
run_fail("${IFC_TOOL}" extract -o invalid-out unsorted.ifc --all)
run_fail("${IFC_TOOL}" extract -o invalid-out bad-path.ifc --all)
run_fail("${IFC_TOOL}" extract -o invalid-out overlap.ifc --all)

# Creation rejects a filepath that extraction cannot reproduce beneath its output root.
file(MAKE_DIRECTORY "${WORK}/path-parent" "${WORK}/path-work")
run_ok("${CMAKE_COMMAND}" -E copy "${WORK}/m.ifc" "${WORK}/path-parent/m.ifc")
run_fail_in("${WORK}/path-work" "${IFC_TOOL}" archive -o "${WORK}/path.ifc" ../path-parent/m.ifc)

# A pre-existing junction in an archive-controlled component cannot redirect extraction.
file(MAKE_DIRECTORY "${WORK}/junction-source/nested" "${WORK}/junction-out" "${WORK}/junction-outside")
run_ok("${CMAKE_COMMAND}" -E copy "${WORK}/m.ifc" "${WORK}/junction-source/nested/m.ifc")
run_ok_in("${WORK}/junction-source" "${IFC_TOOL}" archive -o "${WORK}/junction.ifc" nested/m.ifc)
make_fixture(junction "${WORK}/junction-outside" "${WORK}/junction-out/nested")
run_fail("${IFC_TOOL}" extract -o junction-out junction.ifc --all)
if(EXISTS "${WORK}/junction-outside/m.ifc")
  message(FATAL_ERROR "extract followed a junction outside the output root")
endif()

file(GLOB_RECURSE temporary_files "${WORK}/*.tmp")
if(temporary_files)
  message(FATAL_ERROR "archive operations left temporary files behind: ${temporary_files}")
endif()

message(STATUS "ifc archive/extract round-trip: all checks passed")
