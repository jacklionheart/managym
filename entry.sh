#!/bin/bash
set -e

echo "Running tests with AddressSanitizer enabled..."
cd build
ctest --output-on-failure
