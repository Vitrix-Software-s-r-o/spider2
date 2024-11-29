#!/bin/bash

echo "Running asan tests"
cd "/build-asan"
ctest --verbose --output-on-failure

exit_code=$?
if [ $exit_code -ne 0 ]; then
    exit $exit_code
fi

echo "Running ubsan tests"
cd "/build-ubsan"
ctest --verbose --output-on-failure
exit_code=$?
if [ $exit_code -ne 0 ]; then
    exit $exit_code
fi

exit 0