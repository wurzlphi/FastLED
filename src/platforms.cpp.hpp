/// @file platforms.cpp
/// Platform-specific functions and variables

/// Disables pragma messages and warnings
#define FASTLED_INTERNAL

// Removed duplicate weak definition of timer_millis for ATtiny1604.
// The variable is already defined in avr_millis_timer_null_counter.hpp when needed,
// so redefining it here caused multiple-definition linkage errors.

// Fix for ATtiny1604 - provide weak timer_millis symbol when building normally.
// When compiling using FASTLED_ALL_SRC, avr_millis_timer_null_counter.hpp already
// provides this symbol, so we skip the definition here to avoid a duplicate.
#if defined(__AVR_ATtiny1604__) && !defined(FASTLED_ALL_SRC)
#ifdef __cplusplus
extern "C" {
#endif
__attribute__((weak)) volatile unsigned long timer_millis = 0;
#ifdef __cplusplus
}
#endif
#endif

// Interrupt handlers cannot be defined in the header.
// They must be defined as C functions, or they won't
// be found (due to name mangling), and thus won't
// override any default weak definition.
#if defined(NRF52_SERIES)

    #include "platforms/arm/nrf52/led_sysdefs_arm_nrf52.h"
    #include "platforms/arm/nrf52/arbiter_nrf52.h"

    uint32_t isrCount;

    #ifdef __cplusplus
        extern "C" {
    #endif
            // NOTE: Update platforms.cpp in root of FastLED library if this changes        
            #if defined(FASTLED_NRF52_ENABLE_PWM_INSTANCE0)
                void PWM0_IRQHandler(void) { ++isrCount; PWM_Arbiter<0>::isr_handler(); }
            #endif
            #if defined(FASTLED_NRF52_ENABLE_PWM_INSTANCE1)
                void PWM1_IRQHandler(void) { ++isrCount; PWM_Arbiter<1>::isr_handler(); }
            #endif
            #if defined(FASTLED_NRF52_ENABLE_PWM_INSTANCE2)
                void PWM2_IRQHandler(void) { ++isrCount; PWM_Arbiter<2>::isr_handler(); }
            #endif
            #if defined(FASTLED_NRF52_ENABLE_PWM_INSTANCE3)
                void PWM3_IRQHandler(void) { ++isrCount; PWM_Arbiter<3>::isr_handler(); }
            #endif
    #ifdef __cplusplus
        }
    #endif

#endif // defined(NRF52_SERIES)



// FASTLED_NAMESPACE_BEGIN
// FASTLED_NAMESPACE_END
