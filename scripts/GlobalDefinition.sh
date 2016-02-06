KERNEL_VERSION=4.3.0+

# for Simics
viper0=4022
viper1=5022
fxr=/mnt/fabric/fxr
export LD_LIBRARY_PATH=${fxr}/simics/SynopsisInstructionSetSimulator/lib
export SNPSLMD_LICENSE_FILE="26586@synopsys03p.elic.intel.com"
export LM_PROJECT=”FDO”
LOCK_DIR=/run/lock
LOCK_FILE=${LOCK_DIR}/${myname}.lock
LOCK_TIMEOUT=900 # 900 = 60 * 15 = 15min
MAX_SIMICS_WAIT=180 # = 3 * 60 = 3min
