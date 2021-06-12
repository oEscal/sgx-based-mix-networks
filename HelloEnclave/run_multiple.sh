#!/bin/bash

mix_size=10
first_port=4000
port=$first_port
ports=($(seq $first_port $(($first_port + $mix_size - 1))))
echo ${#ports[@]}

mkdir logs

for i in $(seq 0 $(($mix_size - 1))); do
   echo ${ports[i]} ${ports[(i-1)%mix_size]} ${ports[(i+1)%mix_size]}
   ./app ${ports[i]} ${ports[(i-1)%mix_size]} ${ports[(i+1)%mix_size]} 5555 > logs/app_$i.log &
   port=$((port + 1))
done
# ./app $port $first_port 5555 >> app.log &
