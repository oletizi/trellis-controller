#include "InputLayerFactory.h"

// Include platform-specific implementations
#ifdef ARDUINO
#include "NeoTrellisInputLayer.h"
#else
#include "CursesInputLayer.h" 
#include "MockInputLayer.h"
#endif

#include <stdexcept>

// Platform detection includes
#ifdef ARDUINO
#include <Arduino.h>
#include <Wire.h>
#else
#include <cstdlib>
#include <ncurses.h>
#endif

std::unique_ptr<InputLayerFactory> InputLayerFactory::createEmbeddedFactory() {
    return std::make_unique<EmbeddedInputLayerFactory>();
}

std::unique_ptr<InputLayerFactory> InputLayerFactory::createSimulationFactory() {
    return std::make_unique<SimulationInputLayerFactory>();
}

std::unique_ptr<InputLayerFactory> InputLayerFactory::createTestingFactory() {
    return std::make_unique<TestingInputLayerFactory>();
}

std::unique_ptr<InputLayerFactory> InputLayerFactory::createAutoDetectedFactory(Platform preferredPlatform) {
    Platform detectedPlatform = detectPlatform();
    
    // Use preferred platform if detection is ambiguous
    if (detectedPlatform == Platform::UNKNOWN && preferredPlatform != Platform::UNKNOWN) {
        detectedPlatform = preferredPlatform;
    }
    
    // Fallback to simulation if still unknown
    if (detectedPlatform == Platform::UNKNOWN) {
        detectedPlatform = Platform::SIMULATION;
    }
    
    switch (detectedPlatform) {
        case Platform::EMBEDDED:
            return createEmbeddedFactory();
        case Platform::SIMULATION:
            return createSimulationFactory();
        case Platform::TESTING:
            return createTestingFactory();
        default:
            // Final fallback
            return createSimulationFactory();
    }
}

InputLayerFactory::Platform InputLayerFactory::detectPlatform() {
    // Check for testing environment first (environment variable or explicit request)
    if (isTestingEnvironment()) {
        return Platform::TESTING;
    }
    
    // Check for embedded environment (Arduino platform)
    if (isEmbeddedEnvironment()) {
        return Platform::EMBEDDED;
    }
    
    // Check for simulation environment (desktop with ncurses)
    if (isSimulationEnvironment()) {
        return Platform::SIMULATION;
    }
    
    return Platform::UNKNOWN;
}

const char* InputLayerFactory::platformToString(Platform platform) {
    switch (platform) {
        case Platform::EMBEDDED:   return "Embedded";
        case Platform::SIMULATION: return "Simulation";
        case Platform::TESTING:    return "Testing";
        case Platform::UNKNOWN:    return "Unknown";
        default:                   return "Invalid";
    }
}

std::unique_ptr<IInputLayer> InputLayerFactory::createInputLayerForCurrentPlatform(
    const InputSystemConfiguration& config,
    const InputLayerDependencies& deps,
    Platform preferredPlatform) {
    
    auto factory = createAutoDetectedFactory(preferredPlatform);
    if (!factory) {
        return nullptr;
    }
    
    if (!factory->isAvailable()) {
        return nullptr;
    }
    
    return factory->createInputLayer(config, deps);
}

bool InputLayerFactory::isEmbeddedEnvironment() {
#ifdef ARDUINO
    return true;
#else
    return false;
#endif
}

bool InputLayerFactory::isSimulationEnvironment() {
#ifdef ARDUINO
    return false;
#else
    // Check if we're in a desktop environment with terminal support
    return hasNCursesSupport();
#endif
}

bool InputLayerFactory::isTestingEnvironment() {
#ifdef ARDUINO
    return false;
#else
    // Check for testing environment indicators
    const char* testEnv = std::getenv("TRELLIS_TEST_MODE");
    return testEnv && (strcmp(testEnv, "1") == 0 || strcmp(testEnv, "true") == 0);
#endif
}

bool InputLayerFactory::hasNeoTrellisHardware() {
#ifdef ARDUINO
    // Attempt to detect NeoTrellis hardware on I2C bus
    // This is a simplified check - real implementation would
    // probe the I2C bus for Seesaw devices
    return true; // Assume hardware is available in embedded builds
#else
    return false;
#endif
}

bool InputLayerFactory::hasNCursesSupport() {
#ifdef ARDUINO
    return false;
#else
    // Simple check - could be more sophisticated
    return true; // Assume ncurses is available in desktop builds
#endif
}

// EmbeddedInputLayerFactory implementation
std::unique_ptr<IInputLayer> EmbeddedInputLayerFactory::createInputLayer(
    const InputSystemConfiguration& config,
    const InputLayerDependencies& deps) {
    
#ifdef ARDUINO
    return std::make_unique<NeoTrellisInputLayer>();
#else
    throw std::runtime_error("EmbeddedInputLayerFactory: Cannot create NeoTrellis layer on non-embedded platform");
#endif
}

bool EmbeddedInputLayerFactory::isAvailable() const {
#ifdef ARDUINO
    return hasNeoTrellisHardware();
#else
    return false;
#endif
}

InputSystemConfiguration EmbeddedInputLayerFactory::getRecommendedConfiguration() const {
    return InputSystemConfiguration::forNeoTrellis();
}

// SimulationInputLayerFactory implementation
std::unique_ptr<IInputLayer> SimulationInputLayerFactory::createInputLayer(
    const InputSystemConfiguration& config,
    const InputLayerDependencies& deps) {
    
#ifdef ARDUINO
    throw std::runtime_error("SimulationInputLayerFactory: Cannot create simulation layer on embedded platform");
#else
    return std::make_unique<CursesInputLayer>();
#endif
}

bool SimulationInputLayerFactory::isAvailable() const {
#ifdef ARDUINO
    return false;
#else
    return hasNCursesSupport();
#endif
}

InputSystemConfiguration SimulationInputLayerFactory::getRecommendedConfiguration() const {
    return InputSystemConfiguration::forSimulation();
}

// TestingInputLayerFactory implementation
std::unique_ptr<IInputLayer> TestingInputLayerFactory::createInputLayer(
    const InputSystemConfiguration& config,
    const InputLayerDependencies& deps) {
    
#ifdef ARDUINO
    throw std::runtime_error("TestingInputLayerFactory: Cannot create mock layer on embedded platform");
#else
    return std::make_unique<MockInputLayer>();
#endif
}

bool TestingInputLayerFactory::isAvailable() const {
#ifdef ARDUINO
    return false;
#else
    return true; // Mock layer is always available on desktop
#endif
}

InputSystemConfiguration TestingInputLayerFactory::getRecommendedConfiguration() const {
    return InputSystemConfiguration::forTesting();
}