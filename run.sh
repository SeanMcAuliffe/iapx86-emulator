#!/usr/bin/bash
if [ $# -eq 0 ]; then
  bin/emulator.exe computer_enhance/perfaware/part1/listing_0037_single_register_mov
else
  bin/emulator.exe $1
fi
