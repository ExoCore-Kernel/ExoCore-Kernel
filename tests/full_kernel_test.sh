#!/usr/bin/env bash
set -e

# Full system test for ExoCore Kernel
# - Verifies modules load and kernel boot
# - Generates detailed logs for debugging

ISO="exocore.iso"
BUILD_LOG="tests/full_test_build.log"
BOOT_LOG="tests/full_test_boot.log"
CPU_LOG="tests/full_test_cpu.log"
ARCH_CHOICE="1"


rm -f "$BUILD_LOG" "$BOOT_LOG" "$CPU_LOG"

echo "Building kernel ISO..." | tee "$BUILD_LOG"
printf '%s\n' "$ARCH_CHOICE" | ./build.sh > "$BUILD_LOG" 2>&1

if [ ! -f "$ISO" ]; then
  echo "Build failed. See $BUILD_LOG" >&2
  exit 1
fi

echo "Booting kernel via build.sh" | tee "$BOOT_LOG"
# Use build.sh to run QEMU so kernel modules are loaded consistently
QEMU_EXTRA="-d int,cpu_reset -D $CPU_LOG"
if command -v timeout >/dev/null; then
  printf '%s\n' "$ARCH_CHOICE" | QEMU_EXTRA="$QEMU_EXTRA" timeout 10s ./build.sh run nographic > "$BOOT_LOG" 2>&1
else
  printf '%s\n' "$ARCH_CHOICE" | QEMU_EXTRA="$QEMU_EXTRA" ./build.sh run nographic > "$BOOT_LOG" 2>&1 &
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

