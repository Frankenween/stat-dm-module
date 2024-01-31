#!/bin/bash

echo
echo "Test 5: data transfer correctness"

SIZE=50
SECTORS=$((SIZE * 2))

inp_data=$(mktemp)
dd if=/dev/random of="$inp_data" bs="${SIZE}k" count=1
loop_device=$(losetup --find --show "$inp_data")
echo "$loop_device"

dmsetup create dmp_loop --table "0 $SECTORS dmp $loop_device"

out_data=$(mktemp)

dd if=/dev/mapper/dmp_loop of="$out_data"

echo
echo "In stat:"

for data in /sys/module/dmp/stat/*; do
  echo "$data:"
  cat "$data"
  echo
done

if ! cmp -s "$inp_data" "$out_data"; then
  echo "Test failed! Transfer failed"
  echo "Original file is $inp_data, got file $out_data"
else
  echo "Test passed"
  echo
  rm "$inp_data"
  rm "$out_data"
fi

dmsetup remove dmp_loop
losetup --detach "$loop_device"
