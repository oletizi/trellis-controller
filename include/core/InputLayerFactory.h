#ifndef INPUT_LAYER_FACTORY_H
#define INPUT_LAYER_FACTORY_H

#include "IInputLayer.h"
#include "InputSystemConfiguration.h"
#include <memory>
#include <string>

/**
 * @file InputLayerFactory.h
 * @brief Factory system for creating platform-appropriate input layers
 * 
 * Provides automatic platform detection and creation of the appropriate
 * input layer implementation. Supports embedded (NeoTrellis), simulation
 * (Curses), and testing (Mock) environments with runtime detection.
 * 
 * The factory system enables the core application to work across all
 * platforms without compile-time platform switches or conditional code.
 * Platform detection is performed at runtime based on available resources
 * and environment capabilities.
 * 
 * Usage:
 * ```cpp
 * auto factory = InputLayerFactory::createAutoDetectedFactory();
 * auto inputLayer = factory->createInputLayer(config, deps);
 * ```
 */
class InputLayerFactory {
public:
    /**
     * @brief Platform types supported by the factory
     */
    enum class Platform {
        UNKNOWN = 0,    ///< Platform not detected or unsupported
        EMBEDDED,       ///< NeoTrellis M4 embedded hardware
        SIMULATION,     ///< Desktop simulation with ncurses
        TESTING         ///< Mock input for unit testing
    };
    
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~InputLayerFactory() = default;
    
    /**
     * @brief Create input layer for this platform
     * 
     * Creates and returns a platform-specific input layer implementation
     * configured with the provided parameters. The returned input layer
     * is ready for initialization.
     * 
     * @param config Initial configuration for the input layer
     * @param deps Dependencies required for operation
     * @return Unique pointer to created input layer, or nullptr on failure
     */
    virtual std::unique_ptr<IInputLayer> createInputLayer(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps) = 0;
    
    /**
     * @brief Get platform type this factory creates
     * 
     * @return Platform type for this factory
     */
    virtual Platform getPlatform() const = 0;
    
    /**
     * @brief Get human-readable name for this platform
     * 
     * @return Platform-specific name (e.g., "NeoTrellis M4", "Simulation")
     */
    virtual const char* getPlatformName() const = 0;
    
    /**
     * @brief Check if this factory can create input layers in current environment
     * 
     * Verifies that all required dependencies, hardware, and resources
     * are available for this platform type. Should be called before
     * attempting to create input layers.
     * 
     * @return true if input layers can be created successfully
     */
    virtual bool isAvailable() const = 0;
    
    /**
     * @brief Get recommended configuration for this platform
     * 
     * Returns a configuration optimized for this platform's capabilities
     * and constraints. Can be used as a starting point and customized
     * as needed for specific applications.
     * 
     * @return Platform-optimized input system configuration
     */
    virtual InputSystemConfiguration getRecommendedConfiguration() const = 0;
    
    // Static factory methods for platform creation
    
    /**
     * @brief Create factory for embedded NeoTrellis M4 platform
     * 
     * @return Factory that creates NeoTrellisInputLayer instances
     */
    static std::unique_ptr<InputLayerFactory> createEmbeddedFactory();
    
    /**
     * @brief Create factory for desktop simulation platform
     * 
     * @return Factory that creates CursesInputLayer instances
     */
    static std::unique_ptr<InputLayerFactory> createSimulationFactory();
    
    /**
     * @brief Create factory for unit testing platform
     * 
     * @return Factory that creates MockInputLayer instances
     */
    static std::unique_ptr<InputLayerFactory> createTestingFactory();
    
    /**
     * @brief Create factory with automatic platform detection
     * 
     * Attempts to detect the current runtime environment and return
     * the most appropriate factory. Detection order:
     * 1. Testing environment (if explicitly requested)
     * 2. Embedded hardware (if I2C and NeoTrellis are available)
     * 3. Simulation environment (if ncurses is available)
     * 4. Fallback to simulation if detection fails
     * 
     * @param preferredPlatform Preferred platform if detection is ambiguous
     * @return Factory for the detected or preferred platform
     */
    static std::unique_ptr<InputLayerFactory> createAutoDetectedFactory(
        Platform preferredPlatform = Platform::UNKNOWN);
    
    /**
     * @brief Detect current platform environment
     * 
     * Examines the runtime environment to determine the most appropriate
     * platform type. Used by createAutoDetectedFactory() but can also
     * be called independently for diagnostic purposes.
     * 
     * @return Detected platform type, or UNKNOWN if detection fails
     */
    static Platform detectPlatform();
    
    /**
     * @brief Get string representation of platform type
     * 
     * @param platform Platform type to convert
     * @return Human-readable platform name
     */
    static const char* platformToString(Platform platform);
    
    /**
     * @brief Create input layer directly without factory
     * 
     * Convenience method that combines platform detection and input layer
     * creation in a single call. Equivalent to:
     * ```cpp
     * auto factory = createAutoDetectedFactory();
     * return factory->createInputLayer(config, deps);
     * ```
     * 
     * @param config Input system configuration
     * @param deps Input layer dependencies
     * @param preferredPlatform Preferred platform for ambiguous cases
     * @return Created input layer, or nullptr on failure
     */
    static std::unique_ptr<IInputLayer> createInputLayerForCurrentPlatform(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps,
        Platform preferredPlatform = Platform::UNKNOWN);

protected:
    /**
     * @brief Default constructor for derived classes
     */
    InputLayerFactory() = default;

private:
    // Platform detection helpers
    static bool isEmbeddedEnvironment();
    static bool isSimulationEnvironment();
    static bool isTestingEnvironment();
    static bool hasNeoTrellisHardware();
    static bool hasNCursesSupport();
};

/**
 * @brief Factory implementation for embedded NeoTrellis M4 platform
 */
class EmbeddedInputLayerFactory : public InputLayerFactory {
public:
    std::unique_ptr<IInputLayer> createInputLayer(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps) override;
    
    Platform getPlatform() const override { return Platform::EMBEDDED; }
    const char* getPlatformName() const override { return "NeoTrellis M4"; }
    bool isAvailable() const override;
    InputSystemConfiguration getRecommendedConfiguration() const override;
};

/**
 * @brief Factory implementation for desktop simulation platform
 */
class SimulationInputLayerFactory : public InputLayerFactory {
public:
    std::unique_ptr<IInputLayer> createInputLayer(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps) override;
    
    Platform getPlatform() const override { return Platform::SIMULATION; }
    const char* getPlatformName() const override { return "Desktop Simulation"; }
    bool isAvailable() const override;
    InputSystemConfiguration getRecommendedConfiguration() const override;
};

/**
 * @brief Factory implementation for unit testing platform
 */
class TestingInputLayerFactory : public InputLayerFactory {
public:
    std::unique_ptr<IInputLayer> createInputLayer(
        const InputSystemConfiguration& config,
        const InputLayerDependencies& deps) override;
    
    Platform getPlatform() const override { return Platform::TESTING; }
    const char* getPlatformName() const override { return "Unit Testing"; }
    bool isAvailable() const override;
    InputSystemConfiguration getRecommendedConfiguration() const override;
};

#endif // INPUT_LAYER_FACTORY_H