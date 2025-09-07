#!/usr/bin/env bash
set -euo pipefail

# --- CONFIG ---
BASE="${BASE:-$HOME/Desktop/BBB_SQUARE_SCRIPT}"
BBB_USER="${BBB_USER:-debian}"
BBB_IP="${BBB_IP:-192.168.129.2}"
BBB_SD="${BBB_SD:-/media/debian/sd}"

PASMPATH="${PASMPATH:-$BASE/am335x_pru_package/pru_sw/utils/pasm}"
APP_INC="${APP_INC:-$BASE/am335x_pru_package/pru_sw/app_loader/include}"
APP_LIB="${APP_LIB:-$BASE/am335x_pru_package/pru_sw/app_loader/lib}"

HW="$BASE/CppScripts/BBBhw/" #directorios con scrips

//compilador
echo "Compilando PRU (.p → .bin)"
if [[ ! -x "$PASMPATH" ]]; then
  command -v pasm >/dev/null || { 
    echo "No encuentro pasm"; exit 1; 
  }
  PASMPATH="$(command -v pasm)"
fi
pushd "$HW" >/dev/null
"$PASMPATH" -b PRUassTestScript.p
#"$PASMPATH" -b PRUassTrigSigScript.p

echo "Compilando host GPIO.cpp (cruzado)"
arm-linux-gnueabihf-g++ -static \
  -I"$APP_INC" -L"$APP_LIB" \
  "$HW/GPIO.cpp" -lprussdrv -o "$HW/GPIO"

echo "Montando SD"
ssh "$BBB_USER@$BBB_IP" "sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 '$BBB_SD' || true"

echo "Subida de archivos"
scp "$HW"/PRUassTestScript.bin "$HW"/PRUassTrigSigScript.bin "$BBB_USER@$BBB_IP:$BBB_SD/"

echo "Subiendo ejecutable host GPIO"
scp "$HW/GPIO" "$BBB_USER@$BBB_IP:~/"

echo "Verificación"
ssh "$BBB_USER@$BBB_IP" "ls -lh '$BBB_SD'/*.bin ~/GPIO"
popd >/dev/null
echo "OK."
