// Minimal test application to verify flashing process works
// This app simply blinks LEDs to confirm device is responsive

#include <stdint.h>
#include <string.h>

// SAMD51 register definitions
#define PORT_GROUP_0  ((volatile uint32_t*)0x41008000)  // Port A
#define PORT_GROUP_1  ((volatile uint32_t*)0x41008080)  // Port B

// Port registers offsets (in 32-bit words)
#define PORT_DIR        0x00
#define PORT_DIRCLR     0x01
#define PORT_DIRSET     0x02
#define PORT_OUT        0x04
#define PORT_OUTCLR     0x05
#define PORT_OUTSET     0x06

// LED on PA23 (built-in LED on many SAMD51 boards)
#define LED_PIN 23
#define LED_PORT PORT_GROUP_0

// Simple delay function
void delay_ms(uint32_t ms) {
    // Approximate delay using busy-wait
    // At 120MHz, this is roughly calibrated
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < 30000; j++) {
            __asm__("nop");
        }
    }
}

// Initialize basic GPIO for LED
void init_led() {
    // Set PA23 as output
    LED_PORT[PORT_DIRSET] = (1 << LED_PIN);
}

// Toggle LED
void toggle_led() {
    static bool led_state = false;
    if (led_state) {
        LED_PORT[PORT_OUTCLR] = (1 << LED_PIN);
    } else {
        LED_PORT[PORT_OUTSET] = (1 << LED_PIN);
    }
    led_state = !led_state;
}

// Simple UART initialization for debugging
void init_uart() {
    // Basic UART setup on SERCOM5 (typical for SAMD51)
    // PA22 = TX, PA23 = RX
    
    // Enable SERCOM5 clock
    volatile uint32_t* MCLK_APBDMASK = (volatile uint32_t*)0x40000824;
    *MCLK_APBDMASK |= (1 << 3);  // Enable SERCOM5
    
    // Configure GCLK for SERCOM5
    volatile uint32_t* GCLK_PCHCTRL = (volatile uint32_t*)0x40001C00;
    GCLK_PCHCTRL[35] = 0x40;  // SERCOM5 CORE clock, use GCLK0
    
    // Basic UART configuration would go here
    // For now, we'll just blink the LED
}

// Send a character (stub for now)
void uart_putc(char c) {
    // Would send character via UART
    (void)c;
}

// Send a string (stub for now)
void uart_puts(const char* str) {
    while (*str) {
        uart_putc(*str++);
    }
}

// Main entry point
int main() {
    // Initialize LED
    init_led();
    
    // Initialize UART for debug output
    init_uart();
    
    // Send startup message
    uart_puts("Minimal Test App Starting\r\n");
    
    // Main loop - just blink LED
    uint32_t counter = 0;
    while (1) {
        // Toggle LED
        toggle_led();
        
        // Delay
        delay_ms(500);
        
        // Increment counter
        counter++;
        
        // Every 10 blinks, send a message
        if (counter % 10 == 0) {
            uart_puts("Still alive...\r\n");
        }
    }
    
    return 0;
}

// Required interrupt handlers (minimal stubs)
extern "C" void NMI_Handler() { while(1); }
extern "C" void HardFault_Handler() { 
    // Rapid blink on hard fault
    init_led();
    while(1) {
        toggle_led();
        delay_ms(100);
    }
}
extern "C" void SVC_Handler() { while(1); }
extern "C" void PendSV_Handler() { while(1); }
extern "C" void SysTick_Handler() { }