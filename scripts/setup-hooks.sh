#!/bin/bash
# setup-hooks.sh - Set up git hooks for the project

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
HOOKS_DIR="$PROJECT_ROOT/.git/hooks"

print_color "$YELLOW" "🪝 Setting up git hooks..."

# Create hooks directory if it doesn't exist
mkdir -p "$HOOKS_DIR"

# Create pre-commit hook
cat > "$HOOKS_DIR/pre-commit" << 'EOF'
#!/bin/bash
# Pre-commit hook to check code formatting

export PATH="/opt/homebrew/opt/llvm@18/bin:$PATH"

# Get the directory of this script
HOOK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HOOK_DIR/../.." && pwd)"

echo "🔍 Checking code formatting..."

# Run format check
if ! "$REPO_ROOT/scripts/check-format.sh"; then
    echo ""
    echo "❌ Code formatting check failed!"
    echo "Please run one of the following to fix formatting:"
    echo "  ./scripts/apply-format.sh"
    echo "  ./scripts/check-format.sh -f"
    echo ""
    echo "Or to skip this check (not recommended):"
    echo "  git commit --no-verify"
    echo ""
    exit 1
fi

echo "✅ Code formatting check passed!"
EOF

# Make pre-commit hook executable
chmod +x "$HOOKS_DIR/pre-commit"

print_color "$GREEN" "✅ Pre-commit hook installed successfully!"
print_color "$YELLOW" "The hook will check code formatting before each commit."
print_color "$YELLOW" "To skip the hook (not recommended): git commit --no-verify"
