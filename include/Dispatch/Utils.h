//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#ifndef DISPATCH_ASSERT

#include <cassert>

#ifdef NDEBUG
#define DISPATCH_ASSERT(...) /* DISPATCH_ASSERT */
#else

#define DISPATCH_ASSERT(EXP, MSG)           \
do {                                        \
    assert((EXP) && (MSG));                 \
} while (0)

#endif // NDEBUG

#endif // DISPATCH_ASSERT
