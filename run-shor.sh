#!/bin/bash

export OMP_NUM_THREADS=4

echo "=====> Trying 121 ..."
/usr/bin/time -p ./shor 32769

echo "=====> Trying 169 ..."
/usr/bin/time -p ./shor 2147483645

