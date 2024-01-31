#!/bin/bash

echo
echo "Test 3: simple zero device, writes only"

block_sizes=(4096 15 3000 5000)
count_numbers=(4 3 5 3)

SIZE=4096
dmsetup create zero1 --table "0 $SIZE zero"
dmsetup create dmp1 --table "0 $SIZE dmp /dev/mapper/zero1"

for i in "${!block_sizes[@]}"; do
    dd if=/dev/random of=/dev/mapper/dmp1   bs="${block_sizes[i]}" count="${count_numbers[i]}"
done

echo
echo "In stat:"

for data in /sys/module/dmp/stat/*; do
  echo "$data:"
  cat "$data"
  echo
done

dmsetup remove dmp1
dmsetup remove zero1