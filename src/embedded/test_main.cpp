// Minimal test firmware to verify startup and basic hardware
// This version bypasses all complex initialization and just blinks an LED

#include <stdint.h>

#ifdef __SAMD51J19A__

// GPIO register definitions for SAMD51
typedef struct {
    volatile uint32_t DIR;       // Data Direction
    volatile uint32_t DIRCLR;    // Data Direction Clear
    volatile uint32_t DIRSET;    // Data Direction Set
    volatile uint32_t DIRTGL;    // Data Direction Toggle
    volatile uint32_t OUT;       // Data Output Value
    volatile uint32_t OUTCLR;    // Data Output Value Clear
    volatile uint32_t OUTSET;    // Data Output Value Set
    volatile uint32_t OUTTGL;    // Data Output Value Toggle
    volatile uint32_t IN;        // Data Input Value
    volatile uint32_t CTRL;      // Control
    volatile uint32_t WRCONFIG;  // Write Configuration
    volatile uint32_t EVCTRL;    // Event Control
    volatile uint32_t PMUX[16];  // Peripheral Multiplexing
    volatile uint32_t PINCFG[32]; // Pin Configuration
} PORT_Type;

#define PORTA_BASE   0x41008000UL
#define PORTB_BASE   0x41008080UL
#define PORTA        ((PORT_Type*)PORTA_BASE)
#define PORTB        ((PORT_Type*)PORTB_BASE)

// SysTick register definitions
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

#define SysTick_BASE    0xE000E010UL
#define SysTick         ((SysTick_Type*)SysTick_BASE)

#define SysTick_CTRL_ENABLE_Pos     0U
#define SysTick_CTRL_ENABLE_Msk     (1UL << SysTick_CTRL_ENABLE_Pos)
#define SysTick_CTRL_TICKINT_Pos    1U  
#define SysTick_CTRL_TICKINT_Msk    (1UL << SysTick_CTRL_TICKINT_Pos)
#define SysTick_CTRL_CLKSOURCE_Pos  2U
#define SysTick_CTRL_CLKSOURCE_Msk  (1UL << SysTick_CTRL_CLKSOURCE_Pos)

volatile uint32_t g_tickCount = 0;

// SysTick interrupt handler
extern "C" void SysTick_Handler(void) {
    g_tickCount++;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = g_tickCount;
    while ((g_tickCount - start) < ms) {
        // Wait
    }
}

static void initSysTick() {
    // Configure SysTick for 1ms tick at 48MHz
    SysTick->LOAD = 48000 - 1;  // 48MHz / 1000 = 48000 ticks per ms
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

#endif

// Comprehensive LED test - try multiple pins and turn OFF red LED
int main() {
    // PORT registers for SAMD51
    volatile uint32_t *porta_dir = (volatile uint32_t*)0x41008000;
    volatile uint32_t *porta_out = (volatile uint32_t*)0x41008010;
    volatile uint32_t *portb_dir = (volatile uint32_t*)0x41008080;
    volatile uint32_t *portb_out = (volatile uint32_t*)0x41008090;
    
    // Set multiple pins as outputs on both ports
    *porta_dir = 0xFFFFFFFF;  // All Port A pins as outputs
    *portb_dir = 0xFFFFFFFF;  // All Port B pins as outputs
    
    // First, try to turn OFF the red LED by setting all pins low
    *porta_out = 0x00000000;
    *portb_out = 0x00000000;
    
    // Wait to see if red LED turns off
    volatile uint32_t counter;
    for (counter = 0; counter < 5000000; counter++) {
        __asm("nop");
    }
    
    // Now cycle through pins to find one that controls a visible LED
    uint8_t pin = 0;
    while (1) {
        // Clear all pins
        *porta_out = 0x00000000;
        *portb_out = 0x00000000;
        
        // Set one pin high
        if (pin < 32) {
            *porta_out = (1 << pin);
        } else {
            *portb_out = (1 << (pin - 32));
        }
        
        // Hold for 500ms
        for (counter = 0; counter < 2000000; counter++) {
            __asm("nop");
        }
        
        // Move to next pin
        pin++;
        if (pin >= 64) pin = 0;  // Cycle through all pins
    }
    
    return 0;
}