#!/bin/bash

run_tests() {
    local policy=$1
    local output_file_memory="test_output_mem_$policy.log"
    local output_file_thread="test_output_thread_$policy.log"
    
    echo "========================================"
    echo "Running tests with eviction policy: $policy"
    echo "========================================"
    
    make clean
    
    if ! make EVICTION_POLICY=$policy; then
        echo "Build failed for policy: $policy"
        exit 1
    fi
    
    echo "Running Valgrind Memory Leak Check..."
    if ! valgrind --leak-check=full --track-origins=yes -s --show-leak-kinds=all --show-reachable=yes ./test_app &> $output_file_memory; then
        echo "Valgrind Memory Check failed for policy: $policy"
        exit 1
    fi
    
    if ! grep -q "Tests 0 Failures 0 Ignored" $output_file_memory; then
        echo "Test failures detected for policy: $policy"
        cat $output_file_memory
        exit 1
    fi

    if ! grep -q "All heap blocks were freed -- no leaks are possible" $output_file_memory; then
        echo "Test failures detected for policy: $policy"
        cat $output_file_memory
        exit 1
    fi
    
    echo "Memory Leak Check passed for policy: $policy"
    
    echo "Running Valgrind Helgrind (Thread Error Check)..."
    if ! valgrind --tool=helgrind --history-level=full ./test_app &> $output_file_thread; then
        echo "Valgrind Helgrind Check failed for policy: $policy"
        exit 1
    fi

    if ! grep -q "ERROR SUMMARY: 0 errors from 0 contexts" $output_file_thread; then
        echo "Valgrind Helgrind Check failed for policy: $policy"
        cat $output_file_thread
        exit 1
    fi
    
    echo "========================================"
    echo "Tests passed for eviction policy: $policy"
    echo "========================================"
}

# run tests
run_tests "TTL"
run_tests "LRU"

echo "================================================"
echo "=                                              ="
echo "=                                              ="
echo "= All tests passed for both eviction policies! ="
echo "= No memory leaks or thread issues detected!   ="
echo "=                                              ="
echo "=                                              ="
echo "================================================"

