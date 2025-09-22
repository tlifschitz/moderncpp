
#!/bin/bash
set -e

mkdir -p build

CXX="g++"
CXXFLAGS="-std=c++20 -pthread -O2 -Wall -Wextra"

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
    # Linux: use time
    for src in "${sources[@]}"; do
        exe="${src%.cpp}"
        echo "Benchmarking $exe with time..."
        /usr/bin/time -f "\nElapsed time: %E" "./build/$exe"
    done
fi
echo "Build complete."