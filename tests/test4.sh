#!/bin/bash

echo
echo "Test 4: two devices in parallel"

SIZE=10000
dmsetup create zero1 --table "0 $SIZE zero"
dmsetup create zero2 --table "0 $SIZE zero"

dmsetup create dmp1 --table "0 $SIZE dmp /dev/mapper/zero1"
dmsetup create dmp2 --table "0 $SIZE dmp /dev/mapper/zero2"

dd of=/dev/mapper/dmp1 if=/dev/random bs=7k count=1
dd if=/dev/mapper/dmp2 of=/dev/null   bs=5k count=1

dd if=/dev/mapper/dmp1 of=/dev/null   bs=6k count=7
dd of=/dev/mapper/dmp2 if=/dev/random bs=1k count=1

echo
echo "In stat:"

for data in /sys/module/dmp/stat/*; do
  echo "$data:"
  cat "$data"
  echo
done

dmsetup remove dmp1
dmsetup remove dmp2

dmsetup remove zero1
dmsetup remove zero2