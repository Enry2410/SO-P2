#!/bin/bash

# Default values
NUM_LINES=10
OPERATION_TYPE=""
NUM_USERS=1

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        -l|--lines)
        NUM_LINES="$2"
        shift
        shift
        ;;
        -t|--type)
        OPERATION_TYPE="$2"
        shift
        shift
        ;;
        -u|--users)
        NUM_USERS="$2"
        shift
        shift
        ;;
        *)
        echo "Unknown option: $1"
        exit 1
        ;;
    esac
done

# Validate parameters
if [ -z "$OPERATION_TYPE" ]; then
    echo "Error: Operation Type are required."
    exit 1
fi

# Extract the last digit of OPERATION_TYPE
LAST_DIGIT="${OPERATION_TYPE: -1}"

# Pad the last digit with leading zeros
OPERATION_TYPE=$(printf "%03d" $LAST_DIGIT)

# Generate file name
FILE_NAME="OPE${OPERATION_TYPE}_$(date +"%d%m%Y")_1.data"

# Generate dummy data
for ((i = 1; i <= NUM_LINES; i++)); do
    ID_OPERACION=$(printf "OPE%03d" $i)
    DATE_TIME=$(date +"%d/%m/%Y %H:%M")
    USER_ID=$(printf "USER%03d" $((RANDOM % NUM_USERS + 1)))
    NO_OPERACION=$((RANDOM % 10 + 1))
    AMOUNT=$((RANDOM % 300 - 150))
    STATUS=("Error" "Finalizado" "Correcto")
    STATUS_RANDOM_INDEX=$((RANDOM % ${#STATUS[@]}))
    STATUS_SELECTED=${STATUS[$STATUS_RANDOM_INDEX]}

    echo "${ID_OPERACION};${DATE_TIME};${DATE_TIME};${USER_ID};${OPERATION_TYPE};${NO_OPERACION};${AMOUNT};${STATUS_SELECTED}"
done > "$FILE_NAME"

echo "Data generated and saved to: $FILE_NAME"

