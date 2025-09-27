#!/bin/bash
# check-format.sh - Check if C++ files are properly formatted with clang-format

set -e

# Source common formatting functions and configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/format-common.sh"

# Function to check formatting for a single file
check_file_format() {
    local file=$1
    local temp_file=$(mktemp)
    
    # Format the file to a temporary file
    "$CLANG_FORMAT" "$file" > "$temp_file"
    
    # Compare with original
    if ! diff -u "$file" "$temp_file" >/dev/null 2>&1; then
        print_color "$RED" "✗ $file"
        if [[ "$VERBOSE" == "1" ]]; then
            echo "Differences:"
            diff -u "$file" "$temp_file" | head -20
            echo ""
        fi
        rm "$temp_file"
        return 1
    else
        print_color "$GREEN" "✓ $file"
        rm "$temp_file"
        return 0
    fi
}

# Function to display help
show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Check if C++ files are properly formatted with clang-format."
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  -v, --verbose  Show formatting differences for failing files"
    echo "  -f, --fix      Fix formatting issues (runs apply-format.sh)"
    echo "  --version      Show clang-format version"
    echo ""
    echo "Examples:"
    echo "  $0                 # Check all C++ files"
    echo "  $0 -v              # Check with verbose output"
    echo "  $0 -f              # Check and fix if needed"
    echo ""
}

# Parse command line arguments
VERBOSE=0
FIX_FORMAT=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -f|--fix)
            FIX_FORMAT=1
            shift
            ;;
        --version)
            "$CLANG_FORMAT" --version
            exit 0
            ;;
        *)
            print_color "$RED" "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_color "$YELLOW" "🔍 Checking C++ code formatting..."
    show_clang_format_info
    
    check_clang_format
    
    local files=($(find_cpp_files))
    local failed_files=()
    local total_files=${#files[@]}
    
    if [[ $total_files -eq 0 ]]; then
        print_color "$YELLOW" "No C++ files found to check."
        exit 0
    fi
    
    print_color "$YELLOW" "Checking $total_files files..."
    echo ""
    
    # Check each file
    for file in "${files[@]}"; do
        if ! check_file_format "$file"; then
            failed_files+=("$file")
        fi
    done
    
    echo ""
    local failed_count=${#failed_files[@]}
    local passed_count=$((total_files - failed_count))
    
    if [[ $failed_count -eq 0 ]]; then
        print_color "$GREEN" "🎉 All $total_files files are properly formatted!"
        exit 0
    else
        print_color "$RED" "❌ $failed_count out of $total_files files need formatting fixes."
        print_color "$GREEN" "✅ $passed_count files are correctly formatted."
        echo ""
        
        if [[ "$FIX_FORMAT" == "1" ]]; then
            print_color "$YELLOW" "🔧 Fixing formatting issues..."
            "$SCRIPT_DIR/apply-format.sh"
        else
            print_color "$YELLOW" "To fix formatting issues, run:"
            print_color "$YELLOW" "  $SCRIPT_DIR/apply-format.sh"
            print_color "$YELLOW" "Or use the -f flag:"
            print_color "$YELLOW" "  $0 -f"
        fi
        
        exit 1
    fi
}

# Run main function
main "$@"
