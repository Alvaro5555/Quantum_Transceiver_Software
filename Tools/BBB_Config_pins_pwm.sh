#!/usr/bin/env bash
# bbb_pwm_simple.sh — Configura P8_13, P8_19, P9_14, P9_16
# Usa PERIOD_NS y DUTY_NS (ns). Por defecto: 400 / 100 (como tu ejemplo).
set -euo pipefail

PERIOD_NS="${PERIOD_NS:-400}"
DUTY_NS="${DUTY_NS:-100}"

# pin, chip, ch
MAP=(
  "P8_13 7 0"
  "P8_19 7 1"
  "P9_14 4 0"
  "P9_16 4 1"
)

ensure_pwm_mode() {
  local pin="$1"
  local mode
  mode="$(config-pin -q "$pin" 2>/dev/null | awk '{print $2}' || true)"
  if [[ "$mode" != "pwm" ]]; then
    echo "config-pin $pin pwm"
    config-pin "$pin" pwm
  fi
}

export_if_needed() {
  local chip="$1" ch="$2" dir="$3"
  if [[ -d "$dir" ]]; then return; fi
  # también soporta pwm$ch si no existe pwm-CHIP:CH
  if [[ -d "/sys/class/pwm/pwmchip${chip}/pwm${ch}" ]]; then return; fi
  echo "$ch" | sudo tee "/sys/class/pwm/pwmchip${chip}/export" >/dev/null
  # espera a que aparezca
  for _ in {1..10}; do
    [[ -d "$dir" || -d "/sys/class/pwm/pwmchip${chip}/pwm${ch}" ]] && return
    sleep 0.1
  done
}

set_period_duty_enable() {
  local chip="$1" ch="$2"
  local d1="/sys/class/pwm/pwmchip${chip}/pwm-${chip}:${ch}"
  local d2="/sys/class/pwm/pwmchip${chip}/pwm${ch}"
  local d="$d1"; [[ -d "$d" ]] || d="$d2"

  # deshabilita para poder cambiar
  [[ -f "$d/enable" ]] && echo 0 | sudo tee "$d/enable" >/dev/null || true
  echo "$PERIOD_NS" | sudo tee "$d/period" >/dev/null
  echo "$DUTY_NS"   | sudo tee "$d/duty_cycle" >/dev/null
  echo 1 | sudo tee "$d/enable" >/dev/null
}

for entry in "${MAP[@]}"; do
  set -- $entry
  PIN="$1"; CHIP="$2"; CH="$3"

  ensure_pwm_mode "$PIN"

  DIR="/sys/class/pwm/pwmchip${CHIP}/pwm-${CHIP}:${CH}"
  export_if_needed "$CHIP" "$CH" "$DIR"

  set_period_duty_enable "$CHIP" "$CH"
  echo "OK: $PIN -> pwmchip${CHIP}:${CH} (period=$PERIOD_NS ns, duty=$DUTY_NS ns)"
done
