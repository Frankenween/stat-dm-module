#!/bin/bash

echo
echo "Test 2: simple zero device, no operations at all"

SIZE=10000
dmsetup create zero1 --table "0 $SIZE zero"
dmsetup create dmp1 --table "0 $SIZE dmp /dev/mapper/zero1"

# do nothing

echo
echo "In stat:"

for data in /sys/module/dmp/stat/*; do
  echo "$data:"
  cat "$data"
  echo
done

dmsetup remove dmp1
dmsetup remove zero1