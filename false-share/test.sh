
#!/bin/bash
set -e

mkdir -p build

CXX="g++"
CXXFLAGS="-std=c++20 -pthread -O3 -Wall -Wextra -Wno-unknown-warning-option -Wno-interference-size"

sources=("secuencial.cpp" "direct-share.cpp" "false-share.cpp" "no-share.cpp")

for src in "${sources[@]}"; do
    exe="${src%.cpp}"
    echo "Building $exe..."
    $CXX $CXXFLAGS -o "build/$exe" "$src"
done

echo "Measuring execution time..."

if [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS: use hyperfine
    if ! command -v hyperfine &> /dev/null; then
        echo "hyperfine not found. Please install it with 'brew install hyperfine'."
        exit 1
    fi

    cmd="hyperfine --warmup 5 --runs 5"
    for src in "${sources[@]}"; do
        exe="${src%.cpp}"
        cmd+=" ./build/$exe"
    done
    eval "$cmd"

else
    # Linux: use time with readable format
    for src in "${sources[@]}"; do
        exe="${src%.cpp}"
        echo "Benchmarking $exe..."
        
        # Run the program and capture timing info
        output=$(/usr/bin/time -f "TIMING_DATA: %e %U %S %P %M" "./build/$exe" 2>&1)
        
        # Extract program output (everything before TIMING_DATA)
        program_output=$(echo "$output" | grep -v "TIMING_DATA:")
        echo "$program_output"
        
        # Extract and format timing data
        timing_line=$(echo "$output" | grep "TIMING_DATA:" | sed 's/TIMING_DATA: //')
        read elapsed user system cpu_percent memory <<< "$timing_line"
        
        # Convert to milliseconds for better readability
        elapsed_ms=$(echo "$elapsed * 1000" | bc -l 2>/dev/null || echo "scale=1; $elapsed * 1000" | bc)
        user_ms=$(echo "$user * 1000" | bc -l 2>/dev/null || echo "scale=1; $user * 1000" | bc)
        system_ms=$(echo "$system * 1000" | bc -l 2>/dev/null || echo "scale=1; $system * 1000" | bc)
        
        printf "  Elapsed time: %8.1f ms\n" "$elapsed_ms"
        printf "  CPU time:     %8.1f ms (user: %.1f ms, system: %.1f ms)\n" \
               "$(echo "$user_ms + $system_ms" | bc)" "$user_ms" "$system_ms"
        printf "  CPU usage:    %8s%%\n" "$cpu_percent"
        printf "  Memory:       %8s KB\n\n" "$memory"
    done
fi
echo "Build complete."