#!/bin/bash
# FastLED Fast Build Script
# This script allows you to easily set the parallelization level for faster builds

set -e

# Default to 2x CPU cores for aggressive parallelization
DEFAULT_JOBS=$(($(nproc) * 2))

# Parse command line arguments
PARALLEL_JOBS=${1:-$DEFAULT_JOBS}

# Validate input
if ! [[ "$PARALLEL_JOBS" =~ ^[0-9]+$ ]]; then
    echo "Error: Parallel jobs must be a number"
    echo "Usage: $0 [number_of_jobs]"
    echo "Example: $0 16"
    echo "Default: $DEFAULT_JOBS (2x CPU cores)"
    exit 1
fi

echo "🚀 FastLED Fast Build Mode"
echo "Setting parallel jobs to: $PARALLEL_JOBS"
echo "CPU cores available: $(nproc)"
echo ""

# Set environment variable for the build
export FASTLED_PARALLEL_JOBS=$PARALLEL_JOBS

# Run the tests with the specified parallelization
echo "Running tests with $PARALLEL_JOBS parallel jobs..."
bash test "$@"

echo ""
echo "✅ Build completed with $PARALLEL_JOBS parallel jobs"
