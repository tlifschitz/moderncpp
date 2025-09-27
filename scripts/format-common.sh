#!/bin/bash
# format-common.sh - Common functions and configuration for formatting scripts

# Ensure this script is sourced, not executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "Error: This script should be sourced, not executed directly."
    echo "Usage: source format-common.sh"
    exit 1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CLANG_FORMAT="clang-format"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to check if clang-format is available
check_clang_format() {
    if ! command -v "$CLANG_FORMAT" &> /dev/null; then
        print_color "$RED" "Error: clang-format not found!"
        print_color "$YELLOW" "Please install clang-format:"
        print_color "$YELLOW" "  macOS: brew install clang-format"
        print_color "$YELLOW" "  Ubuntu/Debian: sudo apt-get install clang-format"
        print_color "$YELLOW" "  Or use a specific version like clang-format-14"
        exit 1
    fi
}

# Function to find C++ files
find_cpp_files() {
    find "$PROJECT_ROOT" \
        -type f \
        \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" -o -name "*.cc" -o -name "*.cxx" \) \
        ! -path "*/build/*" \
        ! -path "*/.git/*" \
        ! -path "*/.*" \
        | sort
}

# Function to show clang-format version info
show_clang_format_info() {
    echo "Project root: $PROJECT_ROOT"
    echo "Using clang-format: $($CLANG_FORMAT --version)"
    echo ""
}

enforce_clang_version() {
    local required_version=$1
    local current_version=$($CLANG_FORMAT --version | grep -oE '[0-9]+' | head -1)

    if [[ "$current_version" -lt "$required_version" ]]; then
        print_color "$RED" "Error: clang-format version $required_version or higher is required."
        exit 1
    fi
}
