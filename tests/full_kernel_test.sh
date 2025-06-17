#!/usr/bin/env bash
set -e

# Full system test for ExoCore Kernel
# - Verifies modules load and kernel boot
# - Generates detailed logs for debugging

ISO="exocore.iso"
BUILD_LOG="tests/full_test_build.log"
BOOT_LOG="tests/full_test_boot.log"
CPU_LOG="tests/full_test_cpu.log"

rm -f "$BUILD_LOG" "$BOOT_LOG" "$CPU_LOG"

if [ ! -f "$ISO" ]; then
  echo "Kernel ISO not found. Building..." | tee "$BUILD_LOG"
  ./build.sh > "$BUILD_LOG" 2>&1 <<EOF2
1
EOF2
fi

if [ ! -f "$ISO" ]; then
  echo "Build failed. See $BUILD_LOG" >&2
  exit 1
fi

echo "Booting kernel via QEMU" | tee "$BOOT_LOG"
# Run QEMU with timeout to capture output
if command -v timeout >/dev/null; then
  timeout 10s qemu-system-x86_64 \
    -cdrom "$ISO" \
    -boot order=d \
    -serial stdio \
    -monitor none \
    -no-reboot \
    -display none \
    -d int,cpu_reset 2> "$CPU_LOG" | tee -a "$BOOT_LOG"
else
  qemu-system-x86_64 \
    -cdrom "$ISO" \
    -boot order=d \
    -serial stdio \
    -monitor none \
    -no-reboot \
    -display none \
    -d int,cpu_reset 2> "$CPU_LOG" | tee -a "$BOOT_LOG" &
  PID=$!
  sleep 10
  kill "$PID" 2>/dev/null || true
  wait "$PID" 2>/dev/null || true
fi

SUCCESS=false
if grep -q "ExoCore booted" "$BOOT_LOG" && grep -q "mods_count" "$BOOT_LOG"; then
  SUCCESS=true
fi

if $SUCCESS; then
  echo "\n=== TEST SUCCESS ===" | tee -a "$BOOT_LOG"
else
  echo "\n=== TEST FAILED ===" | tee -a "$BOOT_LOG"
fi

# Display summary
echo "----- Build Log -----"
head -n 20 "$BUILD_LOG"
echo "----- Boot Log -----"
head -n 20 "$BOOT_LOG"
echo "----- CPU Log -----"
head -n 20 "$CPU_LOG"

