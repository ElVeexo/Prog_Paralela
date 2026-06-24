#ifndef __khrplatform_h_
#define __khrplatform_h_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int8_t khronos_int8_t;
typedef uint8_t khronos_uint8_t;
typedef int16_t khronos_int16_t;
typedef uint16_t khronos_uint16_t;
typedef int32_t khronos_int32_t;
typedef uint32_t khronos_uint32_t;
typedef int64_t khronos_int64_t;
typedef uint64_t khronos_uint64_t;

typedef intptr_t khronos_intptr_t;
typedef uintptr_t khronos_uintptr_t;
typedef intptr_t khronos_ssize_t;
typedef uintptr_t khronos_usize_t;

#if defined(_WIN64)
typedef signed long long int khronos_intptr_native_t;
typedef unsigned long long int khronos_uintptr_native_t;
typedef signed long long int khronos_ssize_native_t;
typedef unsigned long long int khronos_usize_native_t;
#elif defined(__GNUC__) && defined(__x86_64__)
typedef signed long int khronos_intptr_native_t;
typedef unsigned long int khronos_uintptr_native_t;
typedef signed long int khronos_ssize_native_t;
typedef unsigned long int khronos_usize_native_t;
#else
typedef signed long int khronos_intptr_native_t;
typedef unsigned long int khronos_uintptr_native_t;
typedef signed long int khronos_ssize_native_t;
typedef unsigned long int khronos_usize_native_t;
#endif

typedef float khronos_float_t;

typedef enum {
    KHRONOS_FALSE = 0,
    KHRONOS_TRUE = 1,
    KHRONOS_BOOLEAN_ENUM_FORCE_SIZE = 0x7FFFFFFF
} khronos_boolean_enum_t;

#ifdef __cplusplus
}
#endif

#endif
