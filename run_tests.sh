#!/bin/bash

make --silent
insmod dmp.ko

if [ ! -d /sys/module/dmp/stat ]
then
  echo "sys/module/dmp/stat does not exist"
  rmmod dmp
  exit 1
fi

for script in ./tests/*.sh; do
  bash "$script"
done

rmmod dmp
make --silent clean