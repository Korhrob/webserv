#!/bin/bash

LOG_FILE="../logs/log"  # Change this to your actual log file
PYTHON_SCRIPT="chunked.py"  # Change this to your Python script

while true; do
    python3 "$PYTHON_SCRIPT"  # Execute the Python script

    # Check the last line of the log file
    if tail -n 100 "$LOG_FILE" | grep -q "\-- BYTES READ -1 --"; then
        echo "Detected -- BYTES READ -1 -- in log file. Stopping execution."
        break
    fi

    sleep 0.1  # Optional: Prevents rapid looping
done
