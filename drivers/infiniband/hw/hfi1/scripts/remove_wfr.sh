#!/bin/bash
echo "Removing wfr"
sed -i 's/WFR_//g' *.c
sed -i 's/WFR_//g' *.h
sed -i 's/DEBUG/CHIP_DEBUG/' chip_registers.h
sed -i 's/##reg##_STATUS/reg##_STATUS/' chip.c
sed -i 's/##reg##_CLEAR/reg##_CLEAR/' chip.c
sed -i 's/##reg##_MASK/reg##_MASK/' chip.c
sed -i 's/_WFR//g' *.c
sed -i 's/_WFR//g' *.h
sed -i 's/WFR/HFI/g' *.c
sed -i 's/WFR/HFI/g' *.h
sed -i 's/qib_init_wfr_funcs/hf1_init_dd/g' *.c

echo "Checking for wfr"
grep -i wfr *.[ch]
