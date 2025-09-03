#!/bin/bash
# Dependency management script for Trellis Controller

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEPS_ROOT="$PROJECT_ROOT/../.trellis-controller-deps"
DEPS_CONFIG="$PROJECT_ROOT/dependencies.json"

echo "Trellis Controller Dependency Setup"
echo "==================================="
echo "Project Root: $PROJECT_ROOT"
echo "Dependencies Root: $DEPS_ROOT"
echo ""

# Function to clone or update a dependency
setup_dependency() {
    local name="$1"
    local url="$2" 
    local commit_hash="$3"
    local local_path="$4"
    
    local full_path="$DEPS_ROOT/$local_path"
    
    if [ -d "$full_path" ]; then
        echo "✓ $name already exists, checking commit..."
        cd "$full_path"
        current_hash=$(git rev-parse HEAD)
        if [ "$current_hash" = "$commit_hash" ]; then
            echo "  → Already at correct commit: $commit_hash"
        else
            echo "  → Updating to commit: $commit_hash"
            git fetch
            git checkout "$commit_hash"
        fi
    else
        echo "⬇ Cloning $name..."
        mkdir -p "$DEPS_ROOT"
        cd "$DEPS_ROOT"
        git clone "$url" "$local_path"
        cd "$local_path"
        echo "  → Checking out commit: $commit_hash"
        git checkout "$commit_hash"
    fi
    
    echo ""
}

# Create dependencies directory
mkdir -p "$DEPS_ROOT"

# Setup each dependency based on dependencies.json
echo "Setting up dependencies..."

# Adafruit Seesaw Library
setup_dependency "Adafruit_Seesaw" \
    "https://github.com/adafruit/Adafruit_Seesaw.git" \
    "985b41efae3d9a8cba12a7b4d9ff0d226f9e0759" \
    "Adafruit_Seesaw"

# Adafruit NeoTrellis M4 Examples
setup_dependency "Adafruit_NeoTrellisM4" \
    "https://github.com/adafruit/Adafruit_NeoTrellisM4.git" \
    "441e2de0ea36bca52896061a6d5aa452931d2172" \
    "Adafruit_NeoTrellisM4"

# Arduino SAMD Core
setup_dependency "Arduino_Core_SAMD" \
    "https://github.com/arduino/ArduinoCore-samd.git" \
    "993398cb7a23a4e0f821a73501ae98053773165b" \
    "ArduinoCore-samd"

echo "✅ All dependencies set up successfully!"
echo ""
echo "Next steps:"
echo "1. Run 'make check' to verify build environment"
echo "2. Run 'make build' to test embedded compilation with new dependencies"
echo "3. Check that firmware size remains reasonable"

# Return to project root
cd "$PROJECT_ROOT"