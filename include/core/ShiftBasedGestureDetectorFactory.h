#ifndef SHIFT_BASED_GESTURE_DETECTOR_FACTORY_H
#define SHIFT_BASED_GESTURE_DETECTOR_FACTORY_H

#include "ShiftBasedGestureDetector.h"
#include "InputStateProcessor.h"
#include "IGestureDetector.h"
#include <memory>

/**
 * @file ShiftBasedGestureDetectorFactory.h
 * @brief Factory for creating ShiftBasedGestureDetector instances
 * 
 * Provides a clean factory interface for integrating ShiftBasedGestureDetector
 * into existing InputController setups. Follows the project's dependency
 * injection patterns and RAII principles.
 */

/**
 * @brief Factory class for creating ShiftBasedGestureDetector instances
 * 
 * Simplifies integration of the new SHIFT-based gesture detection system
 * into existing InputController workflows. Handles dependency management
 * and configuration setup.
 */
class ShiftBasedGestureDetectorFactory {
public:
    /**
     * @brief Create ShiftBasedGestureDetector with InputStateProcessor integration
     * 
     * Creates a fully configured ShiftBasedGestureDetector that works with
     * the modern InputStateProcessor system for optimal performance.
     * 
     * @param stateProcessor Shared InputStateProcessor for state management
     * @return Unique pointer to configured gesture detector
     */
    static std::unique_ptr<IGestureDetector> createWithStateProcessor(
        InputStateProcessor* stateProcessor) {
        return std::make_unique<ShiftBasedGestureDetector>(stateProcessor);
    }
    
    /**
     * @brief Create standalone ShiftBasedGestureDetector
     * 
     * Creates a ShiftBasedGestureDetector without InputStateProcessor integration.
     * Suitable for legacy setups or testing scenarios.
     * 
     * @return Unique pointer to configured gesture detector
     */
    static std::unique_ptr<IGestureDetector> createStandalone() {
        return std::make_unique<ShiftBasedGestureDetector>(nullptr);
    }
    
    /**
     * @brief Create ShiftBasedGestureDetector with custom configuration
     * 
     * Creates a ShiftBasedGestureDetector with specific configuration settings.
     * Useful for embedded vs simulation vs testing environments.
     * 
     * @param stateProcessor Optional InputStateProcessor for integration
     * @param config Custom configuration for the gesture detector
     * @return Unique pointer to configured gesture detector
     */
    static std::unique_ptr<IGestureDetector> createWithConfig(
        InputStateProcessor* stateProcessor,
        const InputSystemConfiguration& config) {
        auto detector = std::make_unique<ShiftBasedGestureDetector>(stateProcessor);
        detector->setConfiguration(config);
        return detector;
    }
};

/**
 * @brief Example integration with InputController
 * 
 * Shows how to integrate ShiftBasedGestureDetector into existing
 * InputController setups for Phase 2 parameter lock functionality.
 */
namespace ShiftBasedGestureDetectorIntegration {
    
    /**
     * @brief Create InputController with SHIFT-based gesture detection
     * 
     * Example setup for modern InputController with ShiftBasedGestureDetector
     * and InputStateProcessor integration.
     * 
     * @param inputLayer Platform-specific input layer
     * @param clock System clock for timing
     * @param debugOutput Optional debug output
     * @param config System configuration
     * @return Configured InputController ready for SHIFT-based parameter locks
     */
    /*
    std::unique_ptr<InputController> createInputController(
        std::unique_ptr<IInputLayer> inputLayer,
        IClock* clock,
        IDebugOutput* debugOutput,
        const InputSystemConfiguration& config) {
        
        // Create InputStateProcessor
        InputStateProcessor::Dependencies processorDeps;
        processorDeps.clock = clock;
        processorDeps.debugOutput = debugOutput;
        auto stateProcessor = std::make_unique<InputStateProcessor>(processorDeps);
        
        // Create ShiftBasedGestureDetector with state processor integration
        auto gestureDetector = ShiftBasedGestureDetectorFactory::createWithStateProcessor(
            stateProcessor.get());
        
        // Create InputController with all dependencies
        InputController::Dependencies controllerDeps;
        controllerDeps.inputLayer = std::move(inputLayer);
        controllerDeps.gestureDetector = std::move(gestureDetector);
        controllerDeps.inputStateProcessor = std::move(stateProcessor);
        controllerDeps.clock = clock;
        controllerDeps.debugOutput = debugOutput;
        
        return std::make_unique<InputController>(std::move(controllerDeps), config);
    }
    */
}

#endif // SHIFT_BASED_GESTURE_DETECTOR_FACTORY_H