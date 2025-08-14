//
// Created by igor on 10/08/2025.
//

#pragma once


#if !defined(__EMSCRIPTEN__)
#if defined(__clang__)
#define LIBIFF_COMPILER_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define LIBIFF_COMPILER_GCC
#elif defined(_MSC_VER)
#define LIBIFF_COMPILER_MSVC
#endif
#else
#define LIBIFF_COMPILER_WASM
#endif
