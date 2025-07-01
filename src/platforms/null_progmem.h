#pragma once

#include <string.h>  // for memcpy

// Guard against PROGMEM redefinition on platforms that have their own definition
#if !defined(PROGMEM) && !defined(__IMXRT1062__) && !defined(__MK20DX128__) && !defined(__MK20DX256__) && !defined(__MK66FX1M0__) && !defined(__MK64FX512__) && !defined(__MKL26Z64__)
#define PROGMEM
#endif

#if !defined(FL_PROGMEM)
// Ensure PROGMEM is available before using it
#if defined(PROGMEM)
#define FL_PROGMEM PROGMEM
#else
// Fallback for platforms without PROGMEM - just use empty attribute
#define FL_PROGMEM
#endif
#endif

// Safe memory access macros to avoid strict aliasing issues
// These use memcpy which is optimized away by the compiler but avoids UB
static inline uint8_t fl_pgm_read_byte_near_safe(const void* addr) {
    uint8_t result;
    memcpy(&result, addr, sizeof(uint8_t));
    return result;
}

static inline uint16_t fl_pgm_read_word_near_safe(const void* addr) {
    uint16_t result;
    memcpy(&result, addr, sizeof(uint16_t));
    return result;
}

static inline uint32_t fl_pgm_read_dword_near_safe(const void* addr) {
    uint32_t result;
    memcpy(&result, addr, sizeof(uint32_t));
    return result;
}

#define FL_PGM_READ_BYTE_NEAR(x) (fl_pgm_read_byte_near_safe(x))
#define FL_PGM_READ_WORD_NEAR(x) (fl_pgm_read_word_near_safe(x))
#define FL_PGM_READ_DWORD_NEAR(x) (fl_pgm_read_dword_near_safe(x))
#define FL_ALIGN_PROGMEM

#define FL_PROGMEM_USES_NULL 1
