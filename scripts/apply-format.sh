#!/bin/bash
# apply-format.sh - Apply clang-format to C++ files in the repository

set -e

# Source common formatting functions and configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/format-common.sh"

# Function to format a single file
format_file() {
    local file=$1
    local temp_file=$(mktemp)
    
    # Format the file to a temporary file first
    "$CLANG_FORMAT" "$file" > "$temp_file"
    
    # Check if there are changes
    if ! diff -u "$file" "$temp_file" >/dev/null 2>&1; then
        # Apply the changes
        mv "$temp_file" "$file"
        print_color "$GREEN" "✓ Formatted: $file"
        
        if [[ "$SHOW_DIFF" == "1" ]]; then
            echo "Changes applied:"
            diff -u "$file.bak" "$file" 2>/dev/null | head -20 || true
            echo ""
        fi
        
        return 0
    else
        # No changes needed
        rm "$temp_file"
        if [[ "$VERBOSE" == "1" ]]; then
            print_color "$BLUE" "- No changes: $file"
        fi
        return 1
    fi
}

# Function to display help
show_help() {
    echo "Usage: $0 [OPTIONS] [FILES...]"
    echo ""
    echo "Apply clang-format to C++ files in the repository."
    echo ""
    echo "Options:"
    echo "  -h, --help       Show this help message"
    echo "  -v, --verbose    Show verbose output (including unchanged files)"
    echo "  -d, --diff       Show differences for changed files"
    echo "  -n, --dry-run    Show what would be formatted without making changes"
    echo "  -c, --check      Check formatting and exit with error if fixes needed"
    echo "  --version        Show clang-format version"
    echo ""
    echo "Examples:"
    echo "  $0                           # Format all C++ files"
    echo "  $0 -v -d                     # Verbose output with diffs"
    echo "  $0 -n                        # Dry run (no changes)"
    echo "  $0 file1.cpp file2.hpp       # Format specific files"
    echo "  $0 -c                        # Check only (CI mode)"
    echo ""
}

# Parse command line arguments
VERBOSE=0
SHOW_DIFF=0
DRY_RUN=0
CHECK_ONLY=0
SPECIFIC_FILES=()

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
        -d|--diff)
            SHOW_DIFF=1
            shift
            ;;
        -n|--dry-run)
            DRY_RUN=1
            shift
            ;;
        -c|--check)
            CHECK_ONLY=1
            shift
            ;;
        --version)
            "$CLANG_FORMAT" --version
            exit 0
            ;;
        -*)
            print_color "$RED" "Unknown option: $1"
            show_help
            exit 1
            ;;
        *)
            # This is a file argument
            SPECIFIC_FILES+=("$1")
            shift
            ;;
    esac
done

# Main execution
main() {
    if [[ "$CHECK_ONLY" == "1" ]]; then
        print_color "$YELLOW" "🔍 Checking C++ code formatting (check-only mode)..."
        exec "$SCRIPT_DIR/check-format.sh"
    elif [[ "$DRY_RUN" == "1" ]]; then
        print_color "$YELLOW" "🔍 Dry run: Checking what would be formatted..."
    else
        print_color "$YELLOW" "🔧 Applying clang-format to C++ files..."
    fi
    
    show_clang_format_info
    
    check_clang_format
    
    enforce_clang_version "18"

    # Determine which files to process
    local files_to_process
    if [[ ${#SPECIFIC_FILES[@]} -gt 0 ]]; then
        files_to_process=("${SPECIFIC_FILES[@]}")
        print_color "$BLUE" "Processing specific files: ${#files_to_process[@]} files"
    else
        # Use array assignment instead of mapfile for better compatibility
        files_to_process=($(find_cpp_files))
        print_color "$BLUE" "Processing all C++ files: ${#files_to_process[@]} files"
    fi
    
    if [[ ${#files_to_process[@]} -eq 0 ]]; then
        print_color "$YELLOW" "No C++ files found to format."
        exit 0
    fi
    
    echo ""
    
    # Process each file
    local formatted_count=0
    local total_count=${#files_to_process[@]}
    
    for file in "${files_to_process[@]}"; do
        if [[ ! -f "$file" ]]; then
            print_color "$RED" "❌ File not found: $file"
            continue
        fi
        
        if [[ "$DRY_RUN" == "1" ]]; then
            # Dry run: just check if file would be changed
            local temp_file=$(mktemp)
            "$CLANG_FORMAT" "$file" > "$temp_file"
            
            if ! diff -u "$file" "$temp_file" >/dev/null 2>&1; then
                print_color "$YELLOW" "Would format: $file"
                ((formatted_count++))
            elif [[ "$VERBOSE" == "1" ]]; then
                print_color "$BLUE" "No changes needed: $file"
            fi
            
            rm "$temp_file"
        else
            # Actually format the file
            if format_file "$file"; then
                ((formatted_count++))
            fi
        fi
    done
    
    echo ""
    
    # Summary
    if [[ "$DRY_RUN" == "1" ]]; then
        if [[ $formatted_count -eq 0 ]]; then
            print_color "$GREEN" "🎉 All $total_count files are already properly formatted!"
        else
            print_color "$YELLOW" "📊 $formatted_count out of $total_count files would be formatted."
            print_color "$YELLOW" "Run without -n/--dry-run to apply changes."
        fi
    else
        if [[ $formatted_count -eq 0 ]]; then
            print_color "$GREEN" "🎉 All $total_count files were already properly formatted!"
        else
            print_color "$GREEN" "✅ Successfully formatted $formatted_count out of $total_count files."
        fi
    fi
    
    # Git status hint
    if [[ "$DRY_RUN" == "0" && $formatted_count -gt 0 ]]; then
        echo ""
        print_color "$BLUE" "💡 Tip: Review changes with 'git diff' before committing."
    fi
}

# Run main function
main "$@"
