#!/bin/bash

for file in $(find . -name "*.tmp"); do
    dest=$(echo $file | sed -e "s/\.tmp$//g")
    mv "$file" "$dest"
done
