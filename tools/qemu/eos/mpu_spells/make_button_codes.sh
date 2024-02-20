#!/bin/bash
# Extract button codes from all supported cameras

echo "" > button_codes.h
for model in 100D 1100D 1200D 450D 500D 550D 50D 5D2 5D3 600D 60D 6D 650D 700D 70D 7D EOSM EOSM2; do
    echo "MPU_BUTTON_CODES($model)"
    python extract_button_codes.py $model 2>log >>button_codes.h 
done

