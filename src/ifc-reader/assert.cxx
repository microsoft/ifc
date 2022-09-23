//
// Microsoft (R) C/C++ Optimizing Compiler Front-End
// Copyright (C) Microsoft. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "runtime-assertions.hxx"

// Lightweight standalone version of C1xx's assertfe
void assertfe(const char* text, const char* file, msvc::LineNumber line)
{
    fprintf(stderr, "assertion failure: ``%s'' in file ``%s'' at line %d\n", text, file, bits::rep(line));
}