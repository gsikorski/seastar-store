#!/bin/bash

for i in $(seq 1 1000000); do
    KEY=$(tr -dc A-Za-z0-9 </dev/urandom | head -c 256; echo)
    VALUE=$(tr -dc A-Za-z0-9 </dev/urandom | head -c 1024; echo)
    wget --method=PUT -q -S -O - "localhost:8080/values?key=${KEY}&value=${VALUE}" &> /dev/null
done
