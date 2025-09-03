/* Minimal startup code for SAMD51 */

#include <stdint.h>

/* Defined by linker script */
extern uint32_t _estack;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sidata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Function prototypes */
void Reset_Handler(void);
void Default_Handler(void);
int main(void);
void __libc_init_array(void);
void SystemInit(void);

/* Cortex-M4 core handlers */
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* SAMD51 peripheral handlers - add as needed */
void PM_Handler(void) __attribute__((weak, alias("Default_Handler")));
void MCLK_Handler(void) __attribute__((weak, alias("Default_Handler")));
void OSCCTRL_Handler(void) __attribute__((weak, alias("Default_Handler")));
void OSC32KCTRL_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SUPC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void WDT_Handler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_Handler(void) __attribute__((weak, alias("Default_Handler")));

/* Vector table */
__attribute__((section(".vectors")))
const void* vector_table[] = {
    &_estack,                    /* Initial stack pointer */
    Reset_Handler,               /* Reset handler */
    NMI_Handler,                 /* NMI handler */
    HardFault_Handler,           /* Hard fault handler */
    MemManage_Handler,           /* Memory management fault */
    BusFault_Handler,            /* Bus fault */
    UsageFault_Handler,          /* Usage fault */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    SVC_Handler,                /* SVC handler */
    DebugMon_Handler,           /* Debug monitor */
    0,                          /* Reserved */
    PendSV_Handler,             /* PendSV handler */
    SysTick_Handler,            /* SysTick handler */
    
    /* External interrupts (SAMD51) - first few */
    PM_Handler,                 /* 0  Power Manager */
    MCLK_Handler,               /* 1  Main Clock */
    OSCCTRL_Handler,            /* 2  OSCCTRL */
    OSC32KCTRL_Handler,         /* 3  32KHz Oscillator */
    SUPC_Handler,               /* 4  Supply Controller */
    WDT_Handler,                /* 5  Watchdog Timer */
    RTC_Handler,                /* 6  Real-Time Counter */
    /* Add more peripheral handlers as needed */
};

/* Reset handler - entry point */
void Reset_Handler(void) {
    // Initialize system first
    SystemInit();
    
    /* Copy initialized data from flash to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    
    /* Clear BSS section */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }
    
    /* Initialize C++ runtime (global constructors) */
    // __libc_init_array(); // Disabled for minimal test
    
    /* Call main */
    main();
    
    /* Infinite loop if main returns */
    while (1) {
        __asm("nop");
    }
}

/* Default handler for unused interrupts */
void Default_Handler(void) {
    // Try to signal we're in a fault handler by toggling a GPIO pin rapidly
    // This will help us identify if we're stuck in an exception
    volatile uint32_t *porta_dir = (volatile uint32_t*)0x41008000;
    volatile uint32_t *porta_out = (volatile uint32_t*)0x41008010;
    volatile uint32_t *porta_outtgl = (volatile uint32_t*)0x4100801C;
    
    // Set PA27 as output (try different pin)
    *porta_dir |= (1 << 27);
    
    // Rapidly toggle to create visible pattern
    volatile uint32_t counter = 0;
    while (1) {
        *porta_outtgl = (1 << 27);  // Toggle PA27
        for (counter = 0; counter < 100000; counter++) {
            __asm("nop");
        }
    }
}

