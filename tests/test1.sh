#!/bin/bash

echo
echo "Test 1: simple zero device, 1K bytes R/W"

SIZE=4096
dmsetup create zero1 --table "0 $SIZE zero"
dmsetup create dmp1 --table "0 $SIZE dmp /dev/mapper/zero1"

dd if=/dev/random      of=/dev/mapper/dmp1   bs=1k count=1
dd if=/dev/mapper/dmp1 of=/dev/null        bs=1k count=1

echo
echo "In stat:"

for data in /sys/module/dmp/stat/*; do
  echo "$data:"
  cat "$data"
  echo
done

dmsetup remove dmp1
dmsetup remove zero1