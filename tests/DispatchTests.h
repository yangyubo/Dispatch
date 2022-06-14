//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#pragma once

#include <chrono>
#include <thread>
#include "doctest.h"
#include "Dispatch/Dispatch.h"

using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::this_thread;

char * dispatch_test_get_large_file();
void dispatch_test_release_large_file(const char *path);
