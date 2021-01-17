// This file is part of the Primal library.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#if defined(_M_AMD64) || defined(_M_IX86) || defined(__amd64) || defined(__i386)
#	ifdef _MSC_VER
#		include <intrin.h>
#	else
#		include <x86intrin.h>
#	endif
#	define PRIMAL_INTRINSICS_SSE 1
#else
#	define PRIMAL_INTRINSICS_SSE 0
#endif
