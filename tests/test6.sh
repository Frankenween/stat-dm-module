#!/bin/bash

echo
echo "Test 6: data transfer to device correctness"

SIZE=50
SECTORS=$((SIZE * 2))

inp_data=$(mktemp)
out_data=$(mktemp)

dd if=/dev/random of="$inp_data" bs="${SIZE}k" count=1
dd if=/dev/zero   of="$out_data" bs="${SIZE}k" count=1

loop_device=$(losetup --find --show "$out_data")

dmsetup create dmp_loop --table "0 $SECTORS dmp $loop_device"

dd of=/dev/mapper/dmp_loop if="$inp_data"

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
