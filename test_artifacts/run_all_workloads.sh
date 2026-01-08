#!/bin/bash

MEMSIM_BIN=./build/memsim
INPUT_DIR=./test_artifacts/input_workloads
OUTPUT_DIR=./test_artifacts/output_logs

mkdir -p "$OUTPUT_DIR"

for workload in "$INPUT_DIR"/*.txt; do
    filename=$(basename "$workload")
    output_file="$OUTPUT_DIR/$filename"

    "$MEMSIM_BIN" < "$workload" > "$output_file"

    echo "Output saved to: $output_file"
    echo
done
