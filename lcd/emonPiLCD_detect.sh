#!/bin/bash

bus=1
if [[ -n $2 ]]; then
    bus=$2
fi

mapfile -t data < <(i2cdetect -y $bus)

for i in $(seq 1 ${#data[@]}); do
    line=(${data[$i]})
    echo ${line[@]:1} | grep -q $1
    if [ $? -eq 0 ]; then
        echo "$1 is present."
        exit 0
    fi
done

echo "Not found."
exit 1
