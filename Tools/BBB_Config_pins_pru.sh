#!/usr/bin/env bash
set -euo pipefail

mode="${1:-all}"   # all | out | in

pins_out=(P8_39 P8_40 P8_41 P8_42 P8_43 P8_44 P8_45 P8_46)  # PRU1 -> R30
pins_in=(P9_28 P9_29 P9_30 P9_31 P9_41 P9_91 P9_42 P9_92)   # PRU0 -> R31 (doble bola 41/42)

do_out() { for p in "${pins_out[@]}"; do config-pin "$p" pruout; done; }
do_in()  { for p in "${pins_in[@]}";  do config-pin "$p" pruin;  done; }

case "$mode" in
  all) do_out; do_in ;;
  out) do_out ;;
  in)  do_in ;;
  *)   echo "Uso: $0 [all|out|in]"; exit 1 ;;
esac

echo "--- Estado ---"
for p in "${pins_out[@]}"; do config-pin -q "$p" || true; done
for p in "${pins_in[@]}";  do config-pin -q "$p" || true; done
