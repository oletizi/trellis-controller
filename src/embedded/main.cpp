#include "StepSequencer.h"
#include "NeoTrellisDisplay.h"
#include "NeoTrellisInput.h"
#include "IClock.h"
#include "DebugSerial.h"

#ifdef __SAMD51J19A__
// Minimal SAMD51 register definitions for SysTick
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_Type;

typedef struct {
    uint32_t RESERVED[834];
    volatile uint32_t ICSR;
} SCB_Type;

// Memory-mapped register addresses for SAMD51
#define SysTick_BASE    0xE000E010UL
#define SCB_BASE        0xE000E000UL
#define SysTick         ((SysTick_Type*)SysTick_BASE)
#define SCB             ((SCB_Type*)SCB_BASE)

// SysTick Control bits
#define SysTick_CTRL_ENABLE_Pos     0U
#define SysTick_CTRL_ENABLE_Msk     (1UL << SysTick_CTRL_ENABLE_Pos)
#define SysTick_CTRL_TICKINT_Pos    1U  
#define SysTick_CTRL_TICKINT_Msk    (1UL << SysTick_CTRL_TICKINT_Pos)
#define SysTick_CTRL_CLKSOURCE_Pos  2U
#define SysTick_CTRL_CLKSOURCE_Msk  (1UL << SysTick_CTRL_CLKSOURCE_Pos)

// SCB ICSR bits
#define SCB_ICSR_PENDSTSET_Pos      26U
#define SCB_ICSR_PENDSTSET_Msk      (1UL << SCB_ICSR_PENDSTSET_Pos)

// Simple SysTick_Config implementation
static inline uint32_t SysTick_Config(uint32_t ticks) {
    if ((ticks - 1UL) > 0xFFFFFFUL) {
        return 1; // Error: reload value impossible
    }
    
    SysTick->LOAD = ticks - 1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
    return 0;
}

// Simple NVIC priority function (stub for now)
static inline void NVIC_SetPriority(int IRQn, uint32_t priority) {
    // TODO: Implement proper NVIC priority setting
    (void)IRQn; (void)priority;
}

#define SysTick_IRQn    (-1)
#endif

// SAMD51 system timer globals
volatile uint32_t g_tickCount = 0;
static uint32_t g_systemCoreClock = 48000000; // 48MHz for SAMD51

// SysTick interrupt handler
extern "C" void SysTick_Handler(void) {
    g_tickCount++;
}

// Embedded clock implementation using SAMD51 SysTick
class EmbeddedClock : public IClock {
public:
    EmbeddedClock() : startTime_(0) {
        initSysTick();
    }
    
    uint32_t getCurrentTime() const override {
        return millis();
    }
    
    void delay(uint32_t milliseconds) override {
        if (milliseconds == 0) return;
        
        uint32_t start = micros();
        while (milliseconds > 0) {
            while (milliseconds > 0 && (micros() - start) >= 1000) {
                milliseconds--;
                start += 1000;
            }
        }
    }
    
    void reset() override {
        startTime_ = g_tickCount;
    }
    
    // Non-virtual destructor for embedded
    ~EmbeddedClock() {}

private:
    uint32_t startTime_;
    
    void initSysTick() {
        // Configure SysTick for 1ms tick interval
        // SysTick_Config sets up the timer and enables the interrupt
        SysTick_Config(g_systemCoreClock / 1000);
        
        // Set priority (lower number = higher priority)
        NVIC_SetPriority(SysTick_IRQn, 3);
    }
    
    uint32_t millis() const {
        return g_tickCount;
    }
    
    uint32_t micros() const {
        uint32_t ticks, ticks2;
        uint32_t pend, pend2;
        uint32_t count, count2;

        ticks2 = SysTick->VAL;
        pend2 = !!(SCB->ICSR & SCB_ICSR_PENDSTSET_Msk);
        count2 = g_tickCount;

        do {
            ticks = ticks2;
            pend = pend2;
            count = count2;
            ticks2 = SysTick->VAL;
            pend2 = !!(SCB->ICSR & SCB_ICSR_PENDSTSET_Msk);
            count2 = g_tickCount;
        } while ((pend != pend2) || (count != count2) || (ticks < ticks2));

        return ((count + pend) * 1000) + 
               (((SysTick->LOAD - ticks) * (1048576 / (g_systemCoreClock / 1000000))) >> 20);
    }
};

// Global instances - embedded style
static EmbeddedClock g_clock;
static NeoTrellisDisplay g_display;
static NeoTrellisInput g_input(&g_clock);
static StepSequencer g_sequencer;

// Application class following dependency injection pattern
class EmbeddedApp {
public:
    EmbeddedApp() : lastTick_(0) {}
    
    void init() {
        // Initialize debug output first
        DEBUG_INIT();
        DEBUG_PRINTLN("=== Trellis Controller Starting ===");
        
        // Initialize hardware
        DEBUG_PRINT("Initializing display... ");
        g_display.init();
        DEBUG_PRINTLN("OK");
        
        DEBUG_PRINT("Initializing input... ");
        g_input.init();
        DEBUG_PRINTLN("OK");
        
        // Configure sequencer with dependencies
        DEBUG_PRINT("Configuring sequencer... ");
        StepSequencer::Dependencies deps;
        deps.clock = &g_clock;
        g_sequencer = StepSequencer(deps);
        
        // Initialize sequencer with 120 BPM, 8 steps
        g_sequencer.init(120, 8);
        DEBUG_PRINTLN("OK");
        
        // Create demo pattern
        DEBUG_PRINT("Setting up demo pattern... ");
        setupDemoPattern();
        DEBUG_PRINTLN("OK");
        
        // Start sequencer
        DEBUG_PRINT("Starting sequencer... ");
        g_sequencer.start();
        DEBUG_PRINTLN("OK");
        
        DEBUG_PRINTLN("=== Initialization Complete ===");
        DEBUG_MEMORY();
        DEBUG_TIMESTAMP("READY");
    }
    
    void run() {
        init();
        
        DEBUG_PRINTLN("Entering main application loop...");
        uint32_t loopCount = 0;
        uint32_t lastStatusTime = g_clock.getCurrentTime();
        
        // Main application loop
        while (true) {
            // Process input events
            handleInput();
            
            // Update sequencer timing
            g_sequencer.tick();
            
            // Update display
            updateDisplay();
            
            // Periodic status updates (every 10 seconds)
            uint32_t currentTime = g_clock.getCurrentTime();
            if (currentTime - lastStatusTime >= 10000) {
                DEBUG_PRINTF("Status: %d loops, %d ms uptime\n", loopCount, currentTime);
                DEBUG_PRINTF("Step: %d/%d, Playing: %s\n", 
                    g_sequencer.getCurrentStep(),
                    8, // step count
                    g_sequencer.isPlaying() ? "YES" : "NO");
                lastStatusTime = currentTime;
            }
            
            loopCount++;
            
            // Small delay to prevent overwhelming the system
            g_clock.delay(10);
        }
    }

private:
    uint32_t lastTick_;
    
    void setupDemoPattern() {
        // Track 0: steps 0, 2, 4, 6
        g_sequencer.toggleStep(0, 0);
        g_sequencer.toggleStep(0, 2);
        g_sequencer.toggleStep(0, 4);
        g_sequencer.toggleStep(0, 6);
        
        // Track 1: steps 1, 3, 5, 7
        g_sequencer.toggleStep(1, 1);
        g_sequencer.toggleStep(1, 3);
        g_sequencer.toggleStep(1, 5);
        g_sequencer.toggleStep(1, 7);
        
        // Track 2: steps 0, 4
        g_sequencer.toggleStep(2, 0);
        g_sequencer.toggleStep(2, 4);
        
        // Track 3: steps 2, 6
        g_sequencer.toggleStep(3, 2);
        g_sequencer.toggleStep(3, 6);
    }
    
    void handleInput() {
        if (!g_input.pollEvents()) return;
        
        ButtonEvent event;
        while (g_input.getNextEvent(event)) {
            // Debug output for button events
            DEBUG_PRINTF("Button [%d,%d] %s at %d ms\n", 
                event.row, event.col,
                event.pressed ? "PRESSED" : "RELEASED",
                event.timestamp);
            
            // Handle button presses to toggle steps
            if (event.pressed && event.row < 4 && event.col < 8) {
                g_sequencer.toggleStep(event.row, event.col);
                DEBUG_PRINTF("Toggled step [%d,%d] -> %s\n",
                    event.row, event.col,
                    g_sequencer.isStepActive(event.row, event.col) ? "ON" : "OFF");
            }
        }
    }
    
    void updateDisplay() {
        // Clear display
        g_display.clear();
        
        // Get current step for highlighting
        uint8_t currentStep = g_sequencer.getCurrentStep();
        bool isPlaying = g_sequencer.isPlaying();
        
        // Update LED states based on sequencer pattern
        for (uint8_t track = 0; track < 4; track++) {
            for (uint8_t step = 0; step < 8; step++) {
                bool stepActive = g_sequencer.isStepActive(track, step);
                bool isCurrentStep = (step == currentStep && isPlaying);
                
                uint8_t r = 0, g = 0, b = 0;
                
                if (stepActive) {
                    // Different colors for different tracks
                    switch (track) {
                        case 0: r = isCurrentStep ? 255 : 64; break;  // Red
                        case 1: g = isCurrentStep ? 255 : 64; break;  // Green  
                        case 2: b = isCurrentStep ? 255 : 64; break;  // Blue
                        case 3: r = g = isCurrentStep ? 255 : 64; break; // Yellow
                    }
                } else if (isCurrentStep) {
                    // Dim white for current step when not active
                    r = g = b = 16;
                }
                
                g_display.setLED(track, step, r, g, b);
            }
        }
        
        g_display.refresh();
    }
};

// Application entry point
int main() {
    EmbeddedApp app;
    app.run();
    return 0;
}