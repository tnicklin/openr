#!/bin/bash

CONFIG_BASE="/config"

# Function to extract the openr_ctrl_port from the JSON config file
get_control_port() {
    CONFIG_FILE="$CONFIG_BASE/$1"
    PORT=$(grep -oP '"openr_ctrl_port"\s*:\s*\K[0-9]+' "$CONFIG_FILE")
    if [ -z "$PORT" ]; then
        echo "Error: Control port not found in $CONFIG_FILE. Using default port 2018."
        PORT=2018
    fi
    echo "$PORT"
}

# Check if the first argument is 'bash'
if [ "$1" = "bash" ]; then
    exec bash
fi

# Check if the first argument is a config file
if [ -n "$1" ]; then
    CONFIG_FILE=$1
    if [ -f "$CONFIG_FILE" ]; then
        echo "Starting Open/R with config file: $CONFIG_FILE"
        CONTROL_PORT=$(get_control_port "$CONFIG_FILE")
        
        # Set the control port as an environment variable
        export OPENR_CTRL_PORT=$CONTROL_PORT
        
        echo "Control port set to: $OPENR_CTRL_PORT"
        echo "Exposing port $OPENR_CTRL_PORT"
        
        # Start Open/R using the config
        exec /scripts/run_openr.sh "$CONFIG_FILE"
    else
        echo "Error: Config file '$CONFIG_FILE' does not exist."
        exit 1
    fi
else
    echo "No config file provided. Starting bash..."
    exec bash
fi

