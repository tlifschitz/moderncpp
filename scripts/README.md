# Scripts Directory

This directory contains utility scripts for the Modern C++ project.

## Formatting Scripts

### `check-format.sh`
Checks if C++ files in the repository are properly formatted according to the project's clang-format configuration.

**Usage:**
```bash
./scripts/check-format.sh [OPTIONS]

Options:
  -h, --help     Show help message
  -v, --verbose  Show formatting differences for failing files
  -f, --fix      Fix formatting issues (runs apply-format.sh)
  --version      Show clang-format version

Examples:
  ./scripts/check-format.sh           # Check all C++ files
  ./scripts/check-format.sh -v        # Check with verbose output
  ./scripts/check-format.sh -f        # Check and fix if needed
```

**Exit codes:**
- `0`: All files are properly formatted
- `1`: Some files need formatting fixes

### `apply-format.sh`
Applies clang-format to C++ files in the repository.

**Usage:**
```bash
./scripts/apply-format.sh [OPTIONS] [FILES...]

Options:
  -h, --help       Show help message
  -v, --verbose    Show verbose output (including unchanged files)
  -d, --diff       Show differences for changed files
  -n, --dry-run    Show what would be formatted without making changes
  -c, --check      Check formatting and exit with error if fixes needed
  --version        Show clang-format version

Examples:
  ./scripts/apply-format.sh                    # Format all C++ files
  ./scripts/apply-format.sh -v -d              # Verbose output with diffs
  ./scripts/apply-format.sh -n                 # Dry run (no changes)
  ./scripts/apply-format.sh file1.cpp file2.hpp # Format specific files
  ./scripts/apply-format.sh -c                 # Check only (CI mode)
```

## Requirements

- **clang-format**: Install via your package manager
  - macOS: `brew install clang-format`
  - Ubuntu/Debian: `sudo apt-get install clang-format`
  - Or use a specific version like `clang-format-14`

## Configuration

The project uses a `.clang-format` file in the root directory that defines the formatting style:
- Based on LLVM style with customizations
- 4-space indentation
- 100-character line limit
- C++20 standard
- Modern C++ formatting preferences

## File Discovery

The scripts automatically find C++ files with these extensions:
- `.cpp`, `.hpp`, `.c`, `.h`, `.cc`, `.cxx`

**Excluded directories:**
- `build/` directories
- `.git/` directory
- Hidden directories (starting with `.`)

## Integration

### Pre-commit Hook
You can set up a pre-commit hook to automatically check formatting:

```bash
# Create .git/hooks/pre-commit
#!/bin/bash
./scripts/check-format.sh
```

### CI/CD Integration
For continuous integration, use the check-only mode:

```bash
./scripts/check-format.sh  # Exit code 1 if formatting needed
# or
./scripts/apply-format.sh -c  # Same as check-format.sh
```

### Examples

Check formatting before committing:
```bash
./scripts/check-format.sh -v
```

Format specific files with diff output:
```bash
./scripts/apply-format.sh -d src/file1.cpp include/file2.hpp
```

Dry run to see what would be changed:
```bash
./scripts/apply-format.sh -n -v
```
