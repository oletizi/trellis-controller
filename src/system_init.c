// SAMD51 System initialization
// Required for proper chip startup

#include <stdint.h>

// SAMD51 System control registers
#define NVMCTRL_CTRLA  (*((volatile uint32_t*)0x41004000))
#define NVMCTRL_CTRLB  (*((volatile uint32_t*)0x41004004))

// Clock system registers
#define OSCCTRL_XOSC0_CTRL (*((volatile uint16_t*)0x40001010))
#define OSCCTRL_XOSC1_CTRL (*((volatile uint16_t*)0x40001024))
#define OSCCTRL_DPLL0_CTRLA (*((volatile uint8_t*)0x4000102C))
#define OSCCTRL_DPLL0_CTRLB (*((volatile uint32_t*)0x40001030))
#define OSCCTRL_DPLL0_RATIO (*((volatile uint32_t*)0x40001034))
#define GCLK_CTRLA (*((volatile uint8_t*)0x40001C00))
#define GCLK_GENCTRL0 (*((volatile uint32_t*)0x40001C20))
#define MCLK_CPUDIV (*((volatile uint8_t*)0x40000804))

void SystemInit(void) {
    // Configure flash wait states for 48MHz operation
    // NVMCTRL->CTRLA.RWS = 2 (2 wait states for 48MHz)
    NVMCTRL_CTRLB = (NVMCTRL_CTRLB & ~0xF) | 0x2;
    
    // Keep the default 48MHz DFLL clock
    // The bootloader should have already configured this
    
    // Enable GCLK0 with DFLL48M as source (should already be done by bootloader)
    // Just ensure CPU divider is 1
    MCLK_CPUDIV = 0x01;
    
    // The bootloader should have configured:
    // - DFLL48M as main clock
    // - USB clock
    // - Other peripherals
    
    // We rely on the bootloader's clock configuration
}