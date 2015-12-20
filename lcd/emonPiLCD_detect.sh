#!/bin/bash

# Utility to detect I2C device
# Run with:
# sudo ./emonPiLCD_detect.sh 27
# Where 27 = hex number I2C address

# Returns true if I2C is detected at address and False if not

# sudo ./emonPiLCD_detect.sh 27 2
# Add a 2 as a second argument to search I2C bus 2, default 1

bus=1
if [[ -n $2 ]]; then
    bus=$2
fi

mapfile -t data < <(i2cdetect -y $bus)

for i in $(seq 1 ${#data[@]}); do
    line=(${data[$i]})
    echo ${line[@]:1} | grep -q $1
    if [ $? -eq 0 ]; then
        echo "True"
        exit 0
    fi
done

echo "False"
