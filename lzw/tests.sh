#!/bin/bash

for i in {1..3}
do
    echo "Ejecución $i:" >> salida.txt
    ./a.out -c english.part_5MB $i >> salida.txt
    echo "" >> salida.txt
done
