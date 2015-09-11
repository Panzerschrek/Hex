#pragma once

// Disable assert, if DEBUG not set
#ifndef DEBUG
#ifndef NDEBUG
#define NDEBUG
#endif
#endif

#include <cassert>

#ifdef DEBUG
#define H_ASSERT(x) (assert(x))
#else // not debug
#define H_ASSERT(x)
#endif
