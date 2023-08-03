// Copyright Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdio.h>
#include <stdlib.h>
#include "ifc/assertions.hxx"

// Lightweight standalone version of ifc_assert. Consumers can replace this.
void ifc_assert(const char* text, const char* file, int line)
{
    fprintf(stderr, "assertion failure: ``%s'' in file ``%s'' at line %d\n", text, file, line);
    exit(-1);
}

void ifc_verify(const char* text, const char* file, int line)
{
    fprintf(stderr, "verify failure: ``%s'' in file ``%s'' at line %d\n", text, file, line);
    exit(-1);
}
