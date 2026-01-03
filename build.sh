#!/usr/bin/env bash
set -e

APT_UPDATED=""
apt_install() {
  if ! command -v apt-get >/dev/null 2>&1; then
    echo "apt-get not available; install packages manually: $*" >&2
    return 1
  fi
  if [ -z "$APT_UPDATED" ]; then
    apt-get update
    APT_UPDATED=1
  fi
  DEBIAN_FRONTEND=noninteractive apt-get install -y "$@"
}

needs_rebuild() {
  local target="$1"; shift
  local src="$1"; shift
  local depfile="$1"; shift
  if [ ! -f "$target" ]; then
    return 0
  fi
  if [ -n "$src" ] && [ "$src" -nt "$target" ]; then
    return 0
  fi
  if [ -n "$depfile" ] && [ -f "$depfile" ]; then
    local deps
    deps=$(sed -e 's/\\//g' "$depfile" | tr '\n' ' ')
    deps=${deps#*:}
    for dep in $deps; do
      [ -z "$dep" ] && continue
      if [ "$dep" = "\\" ]; then
        continue
      fi
      if [ "$dep" -nt "$target" ]; then
        return 0
      fi
    done
  fi
  for dep in "$@"; do
    if [ "$dep" -nt "$target" ]; then
      return 0
    fi
  done
  return 1
}

should_rebuild() {
  local target="$1"; shift
  if [ ! -f "$target" ]; then
    return 0
  fi
  for dep in "$@"; do
    if [ "$dep" -nt "$target" ]; then
      return 0
    fi
  done
  return 1
}

copy_if_different() {
  local src="$1"
  local dest="$2"
  mkdir -p "$(dirname "$dest")"
  if [ -f "$dest" ] && cmp -s "$src" "$dest"; then
    return 0
  fi
  cp "$src" "$dest"
}

MICROPYTHON_CACHE_DIR="/tmp/exocore-build"
MICROPYTHON_CACHE_ISODIR="$MICROPYTHON_CACHE_DIR/isodir"
MICROPYTHON_CACHE_ISO="$MICROPYTHON_CACHE_DIR/exocore.iso"
MICROPYTHON_CHANGED_FILES=()
MICROPYTHON_OTHER_FILES=()
MICROPYTHON_FAST="false"

is_ignored_path() {
  case "$1" in
    build.txt|.build_pref|include/buildinfo.h)
      return 0 ;;
  esac
  return 1
}

is_micropython_path() {
  local path="$1"
  case "$path" in
    run/*.py|run/*/*.py|run/*/*/*.py|init/kernel/init.py|init/kernel/*.py|init/kernel/*/*.py|init/kernel/*/*/*.py)
      return 0 ;;
  esac
  return 1
}

micropython_dest_path() {
  local src="$1"
  case "$src" in
    run/*.py|run/*/*.py|run/*/*/*.py)
      printf 'isodir/boot/%s' "$(basename "$src")"
      return 0 ;;
    init/kernel/init.py)
      printf 'isodir/boot/init/kernel/init.py'
      return 0 ;;
    init/kernel/*)
      local rel="${src#init/kernel/}"
      printf 'isodir/boot/init/kernel/%s' "$rel"
      return 0 ;;
  esac
  return 1
}

refresh_iso_cache() {
  mkdir -p "$MICROPYTHON_CACHE_DIR"
  rm -rf "$MICROPYTHON_CACHE_ISODIR"
  mkdir -p "$MICROPYTHON_CACHE_ISODIR"
  if [ -d isodir ]; then
    cp -a isodir/. "$MICROPYTHON_CACHE_ISODIR/"
  fi
  if [ -f exocore.iso ]; then
    cp exocore.iso "$MICROPYTHON_CACHE_ISO"
  fi
}

if git rev-parse --git-dir >/dev/null 2>&1; then
  while IFS= read -r line; do
    [ -z "$line" ] && continue
    local_status="${line:0:2}"
    path="${line:3}"
    if [[ "$path" == *" -> "* ]]; then
      path="${path##* -> }"
    fi
    path="${path#./}"
    [ -z "$path" ] && continue
    if is_ignored_path "$path"; then
      continue
    fi
    case "$local_status" in
      \?\?|*A*|*D*|*R*)
        MICROPYTHON_OTHER_FILES+=("$path")
        continue
        ;;
    esac
    if is_micropython_path "$path"; then
      MICROPYTHON_CHANGED_FILES+=("$path")
    else
      MICROPYTHON_OTHER_FILES+=("$path")
    fi
  done < <(git status --porcelain --untracked-files=all)

  if [ ${#MICROPYTHON_CHANGED_FILES[@]} -gt 0 ] && [ ${#MICROPYTHON_OTHER_FILES[@]} -eq 0 ]; then
    if [ -d "$MICROPYTHON_CACHE_ISODIR" ]; then
      MICROPYTHON_FAST="true"
      echo "Detected MicroPython-only modifications: ${MICROPYTHON_CHANGED_FILES[*]}"
      echo "Reusing cached kernel artifacts for quick ISO rebuild."
    else
      echo "MicroPython-only modifications detected but no cached ISO available; full rebuild required."
    fi
  elif [ ${#MICROPYTHON_CHANGED_FILES[@]} -gt 0 ] && [ ${#MICROPYTHON_OTHER_FILES[@]} -gt 0 ]; then
    echo "MicroPython changes detected along with other files; performing full rebuild."
  fi
fi

# Build count tracking
VERSION_FILE="version.txt"
COUNT_FILE="build.txt"
PREF_FILE=".build_pref"
[ -f "$VERSION_FILE" ] || echo "0.0.1" > "$VERSION_FILE"
current_major=$(cut -d'.' -f1 "$VERSION_FILE")
if [ -f "$COUNT_FILE" ]; then
    stored_major=$(cut -d':' -f1 "$COUNT_FILE")
    count=$(cut -d':' -f2 "$COUNT_FILE")
else
    stored_major="$current_major"
    count=0
fi
if [ "$stored_major" != "$current_major" ]; then
    # reset count on major version change, try to continue from last tag if exists
    if git rev-parse --git-dir >/dev/null 2>&1; then
        last_tag=$(git describe --tags --abbrev=0 2>/dev/null || true)
        if [ -n "$last_tag" ]; then
            latest_major=$(echo "$last_tag" | sed 's/^v//' | cut -d'.' -f1)
            if [ "$latest_major" = "$current_major" ]; then
                count=$(git show "$last_tag:build.txt" 2>/dev/null | cut -d':' -f2)
            else
                count=0
            fi
        else
            count=0
        fi
    else
        count=0
    fi
fi
count=$((count + 1))
printf "%s:%d" "$current_major" "$count" > "$COUNT_FILE"
if [ -f "$PREF_FILE" ]; then
    CHOIICEV=$(cat "$PREF_FILE")
fi
if [ ! -t 0 ] && [ -z "$CHOIICEV" ]; then
    CHOIICEV="never"
fi
if [ -z "$CHOIICEV" ]; then
    echo "Commit build count to GitHub?"
    select opt in "Just once" "Always" "Never"; do
        case $REPLY in
            1) CHOIICEV="ask"; break ;;
            2) CHOIICEV="always"; echo "$CHOIICEV" > "$PREF_FILE"; break ;;
            3) CHOIICEV="never"; echo "$CHOIICEV" > "$PREF_FILE"; break ;;
        esac
    done
fi
export CHOIICEV
commit_build=false
if [ "$CHOIICEV" = "always" ]; then
    commit_build=true
elif [ "$CHOIICEV" = "ask" ]; then
    read -p "Commit build count? [y/N]: " ans
    [[ $ans =~ ^[Yy]$ ]] && commit_build=true
fi
if $commit_build; then
    git add "$COUNT_FILE" "$PREF_FILE" "$VERSION_FILE" include/buildinfo.h 2>/dev/null || true
    git commit -m "Update build count to $count" 2>/dev/null || true
    if git remote >/dev/null 2>&1; then
        git push origin HEAD 2>/dev/null || true
    fi
fi

# Repository updates are managed via update.sh

if [ "$1" = "clean" ]; then
    rm -f arch/x86/boot.o arch/x86/idt.o arch/x86/user.o \
          kernel/main.o kernel/mem.o kernel/console.o kernel/serial.o \
          kernel/idt.o kernel/panic.o kernel/memutils.o kernel/fs.o kernel/script.o \
          kernel/debuglog.o kernel/syscall.o kernel/micropython.o kernel/mpy_loader.o \
          kernel/mpy_modules.o kernel/modexec.o kernel/vga_draw.o kernel/framebuffer.o kernel/io.o
    rm -f kernel/*.d run/*.d run/userland/*.d run/console_mod.d run/serial_mod.d kernel/micropython.d
    rm -rf isodir run mpbuild
    rm -f run/linkdep.a kernel.bin exocore.iso
    echo "Build artifacts cleaned"
    exit 0
fi

if [ "$MICROPYTHON_FAST" != "true" ]; then
# Fetch and build MicroPython embed port
# Fetch the MicroPython source if missing and keep it up to date
MP_DIR="micropython"
if [ ! -d "$MP_DIR" ]; then
  git clone --depth 1 https://github.com/micropython/micropython.git "$MP_DIR"
fi
# Ensure the Micropython embed port is built
if [ ! -d "$MP_DIR/examples/embedding/micropython_embed" ]; then
  make -C "$MP_DIR/examples/embedding" -f micropython_embed.mk
fi
if [ ! -f "$MP_DIR/examples/embedding/mpconfigport.h" ]; then
  echo "Error: mpconfigport.h not found in $MP_DIR/examples/embedding" >&2
  echo "Micropython fetch or build failed" >&2
  exit 1
fi
# Ensure persistent .mpy loading is enabled
# Enable sys module for MicroPython runtime
if grep -q "MICROPY_PY_SYS" "$MP_DIR/examples/embedding/mpconfigport.h"; then
  sed -i 's/^#define MICROPY_PY_SYS.*/#define MICROPY_PY_SYS (1)/' "$MP_DIR/examples/embedding/mpconfigport.h"
else
  echo "#define MICROPY_PY_SYS (1)" >> "$MP_DIR/examples/embedding/mpconfigport.h"
fi
if grep -q "MICROPY_PY_SYS_PLATFORM" "$MP_DIR/examples/embedding/mpconfigport.h"; then
  sed -i 's/^#define MICROPY_PY_SYS_PLATFORM.*/#define MICROPY_PY_SYS_PLATFORM "exocore"/' "$MP_DIR/examples/embedding/mpconfigport.h"
else
  echo "#define MICROPY_PY_SYS_PLATFORM \"exocore\"" >> "$MP_DIR/examples/embedding/mpconfigport.h"
fi
if ! grep -q "MICROPY_PERSISTENT_CODE_LOAD" "$MP_DIR/examples/embedding/mpconfigport.h"; then
  echo "#define MICROPY_PERSISTENT_CODE_LOAD (1)" >> "$MP_DIR/examples/embedding/mpconfigport.h"
fi
# patch stdout handler to use kernel console
cat > "$MP_DIR/ports/embed/port/mphalport.c" <<'EOF'
#include "console.h"
#include "serial.h"
#include "py/mphal.h"
#include "runstate.h"

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return len;
    for (size_t i = 0; i < len; i++) {
        console_putc(str[i]);
    }
    return len;
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return;
    for (size_t i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\n') console_putc('\r');
        console_putc(c);
    }
}

mp_uint_t mp_hal_stderr_tx_strn(const char *str, size_t len) {
    serial_write(str);
    if (!mp_vga_output) return len;
    for (size_t i = 0; i < len; i++) {
        console_putc(str[i]);
    }
    return len;
}
EOF

# Prepare MicroPython user modules and rebuild the embed port
USERMOD_BASE="$MP_DIR/examples/embedding/micropython_embed/usermod"
USERMOD_DST="$USERMOD_BASE/exocore"
rm -rf "$USERMOD_BASE"
mkdir -p "$USERMOD_DST"
for native_dir in mpymod/*/native; do
  [ -d "$native_dir" ] || continue
  for src in "$native_dir"/*.c "$native_dir"/*.h; do
    [ -f "$src" ] || continue
    dest="$USERMOD_DST/$(basename "$src")"
    cp "$src" "$dest"
    if [[ "$src" == *.c ]]; then
      modname="${src##*/}"
      modname="${modname%.c}"
      base="${modname#mod}"
      if ! grep -q "MP_REGISTER_MODULE" "$dest"; then
        echo "MP_REGISTER_MODULE(MP_QSTR_${base}, ${base}_user_cmodule);" >> "$dest"
      fi
    fi
  done
done

python3 - <<'PY' "$USERMOD_DST"
import re
import sys
from pathlib import Path

dst = Path(sys.argv[1])
c_files = sorted(p for p in dst.iterdir() if p.suffix == '.c')
qstrs = set()
for path in c_files:
    text = path.read_text(encoding='utf-8', errors='ignore')
    qstrs.update(re.findall(r'MP_QSTR_([A-Za-z0-9_]+)', text))

mk_path = dst / 'micropython.mk'
with mk_path.open('w', encoding='utf-8') as mk:
    mk.write('EXOCORE_USERMOD_DIR := $(USERMOD_DIR)\n\n')
    for path in c_files:
        mk.write(f'SRC_USERMOD += $(EXOCORE_USERMOD_DIR)/{path.name}\n')
    include_dir = Path('include').resolve()
    mk.write('\nCFLAGS_USERMOD += -I$(EXOCORE_USERMOD_DIR)')
    mk.write(f' -I{include_dir}\n')
    mk.write('QSTR_DEFS += $(EXOCORE_USERMOD_DIR)/qstrdefs.h\n')

qstr_path = dst / 'qstrdefs.h'
with qstr_path.open('w', encoding='utf-8') as qh:
    qh.write('// Auto-generated by build.sh. Do not edit manually.\n')
    for name in sorted(qstrs):
        qh.write(f'Q({name})\n') 
PY

# Generate table of MicroPython modules for embedding
MPY_MOD_C="kernel/mpy_modules.c"
python3 - "$MPY_MOD_C" <<'PY'
import os, sys, json
out_path = sys.argv[1]
with open(out_path, 'w') as out:
    out.write('#include "mpy_loader.h"\n#include <stddef.h>\n\n')
    entries = []
    for mod in sorted(os.listdir('mpymod')):
        mod_dir = os.path.join('mpymod', mod)
        if not os.path.isdir(mod_dir):
            continue
        manifest = os.path.join(mod_dir, 'manifest.json')
        name = mod
        entry = 'init.py'
        if os.path.exists(manifest):
            data = json.load(open(manifest))
            name = data.get('mpy_import_as', name)
            entry = data.get('mpy_entry') or 'init.py'
        src_path = os.path.join(mod_dir, entry)
        if not os.path.exists(src_path):
            continue
        with open(src_path, 'rb') as f:
            data = f.read()
        var = mod.replace('-', '_')
        out.write(f'static const unsigned char {var}_src[] = {{')
        out.write(','.join(f'0x{b:02x}' for b in data))
        out.write('};\n')
        entries.append((name, var, len(data)))
    out.write('const mpymod_entry_t mpymod_table[] = {\n')
    for name, var, length in entries:
        out.write(f'    {{"{name}", (const char*){var}_src, {length}}},\n')
    out.write('};\n')
    out.write(f'const size_t mpymod_table_count = {len(entries)};\n')
PY

# rebuild embed port to include patched mphalport.c and any user modules
USERMOD_PARENT_ABS=$(cd "$USERMOD_BASE" && pwd)
rm -rf "$MP_DIR/examples/embedding/build-embed"
make -C "$MP_DIR/examples/embedding" -f micropython_embed.mk \
     USERMOD_DIR=micropython_embed/usermod/exocore \
     USER_C_MODULES="$USERMOD_PARENT_ABS"
else
  echo "Skipping MicroPython embed rebuild; using cached kernel for ISO repack."
fi

# 1) Target selection & tool fallback installer
echo "Select target architecture, comma-separated choices:"
echo " 1) native (host)"
echo " 2) x86_64-elf (AMD64)"
read -p "Enter choice [1-2]: " arch_choice

# Module load address must match MODULE_BASE_ADDR in include/config.h
MODULE_BASE=0x00200000

case "$arch_choice" in
  1)
    CC=gcc; LD=ld; ARCH_FLAG=-m64; LDARCH="elf_x86_64";
    QEMU=qemu-system-x86_64; FALLBACK_PKG="build-essential" ;;
  2)
    CC=x86_64-linux-gnu-gcc; LD=x86_64-linux-gnu-ld; ARCH_FLAG=-m64; LDARCH="elf_x86_64";
    QEMU=qemu-system-x86_64; FALLBACK_PKG="binutils-x86-64-linux-gnu gcc-x86-64-linux-gnu" ;;
  *) echo "Invalid choice, bro."; exit 1 ;;
esac

# Use 64-bit objects for modules when the x86_64 toolchain is selected
MODULE_FLAG="$ARCH_FLAG"
if [ "$arch_choice" = "2" ]; then
  MODULE_FLAG="-m64"
fi

# stack safety flags applied to all C compilation
STACK_FLAGS="-mstackrealign -fno-omit-frame-pointer"

# install compiler if missing
if ! command -v "$CC" &>/dev/null; then
  echo "$CC not found, installing packages: $FALLBACK_PKG"
  if ! apt_install $FALLBACK_PKG; then
    echo "Install $FALLBACK_PKG manually" >&2
    exit 1
  fi
fi

# static tools
NASM=nasm
GRUB=grub-mkrescue
: ${QEMU:=qemu-system-i386}

# ensure nasm present
if ! command -v "$NASM" &>/dev/null; then
  echo "nasm missing, installing..."
  apt_install nasm || { echo "Install nasm manually"; exit 1; }
fi

# ensure grub-mkrescue and mtools utilities exist
if ! command -v "$GRUB" &>/dev/null; then
  echo "$GRUB missing, installing grub and dependencies..."
  apt_install grub-pc-bin grub-common xorriso mtools || {
    echo "Install grub-mkrescue and mtools manually"; exit 1; }
fi
if ! command -v mformat &>/dev/null; then
  echo "mtools missing, installing..."
  apt_install mtools || { echo "Install mtools manually"; exit 1; }
fi

# ensure QEMU is installed for runtime testing
if ! command -v "$QEMU" &>/dev/null; then
  echo "$QEMU missing, installing QEMU..."
  apt_install qemu-system || { echo "Install QEMU manually"; exit 1; }
fi


if [ "$MICROPYTHON_FAST" = "true" ]; then
  echo "Updating cached ISO tree with modified MicroPython sources..."
  rm -rf isodir
  mkdir -p isodir
  if [ -d "$MICROPYTHON_CACHE_ISODIR" ]; then
    cp -a "$MICROPYTHON_CACHE_ISODIR/." isodir/
  fi
  for mp_path in "${MICROPYTHON_CHANGED_FILES[@]}"; do
    dest=$(micropython_dest_path "$mp_path") || {
      echo "Warning: could not map MicroPython path $mp_path into ISO" >&2
      continue
    }
    copy_if_different "$mp_path" "$dest"
  done
  echo "Rebuilding ISO from cached kernel..."
  $GRUB -o exocore.iso isodir
  refresh_iso_cache
  if [ "$1" = "run" ]; then
    echo "Booting in QEMU…"
    if [ "$2" = "nographic" ]; then
      $QEMU -cdrom exocore.iso \
           -boot order=d \
           -serial stdio \
           -monitor none \
           -no-reboot \
           -display none \
           ${QEMU_EXTRA:+$QEMU_EXTRA}
    else
      $QEMU -cdrom exocore.iso \
           -boot order=d \
           -serial stdio \
           -monitor none \
           -no-reboot \
           ${QEMU_EXTRA:+$QEMU_EXTRA}
    fi
  else
    echo "Done, use './build.sh run [nographic]' to build & boot"
  fi
  exit 0
fi



# 2) Ensure required directories exist for incremental builds
mkdir -p linkdep run/linkdep_objs run/userland mpbuild isodir/boot/grub


# 3) auto-stub console_getc if you don’t have it
if [ ! -f linkdep/console_getc.c ]; then
  cat > linkdep/console_getc.c << 'EOF'
// auto-generated stub for console_getc
#include "console.h"
char console_getc(void) {
    return 0;  // always return zero, override as needed
}
EOF
  echo "Generated linkdep/console_getc.c stub"
fi

# 4) Compile linkdep/*.c → run/linkdep_objs/*.o
# compile link dependency sources
for src in linkdep/*.c; do
  [ -f "$src" ] || continue
  obj="run/linkdep_objs/$(basename "${src%.c}.o")"
  dep="${obj%.o}.d"
  if needs_rebuild "$obj" "$src" "$dep"; then
    echo "Compiling linkdep $src → $obj"
    $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
        -Iinclude -MMD -MP -MF "$dep" -c "$src" -o "$obj"
  else
    echo "Up to date $obj"
  fi
done

# archive deps so ld only pulls used ones
shopt -s nullglob
DEP_OBJS=( run/linkdep_objs/*.o )
if [ ${#DEP_OBJS[@]} -gt 0 ]; then
  if should_rebuild run/linkdep.a "${DEP_OBJS[@]}"; then
    echo "Creating static archive run/linkdep.a"
    ar rcs run/linkdep.a "${DEP_OBJS[@]}"
  else
    echo "run/linkdep.a is up to date"
  fi
fi
shopt -u nullglob

# 5) Build console and serial stubs for modules
mkdir -p run
echo "Building console stub → run/console_mod.o"
if needs_rebuild run/console_mod.o kernel/console.c run/console_mod.d; then
  $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall \
      -DNO_DEBUGLOG -Iinclude -MMD -MP -MF run/console_mod.d \
      -c kernel/console.c -o run/console_mod.o
else
  echo "run/console_mod.o is up to date"
fi
echo "Building serial stub → run/serial_mod.o"
if needs_rebuild run/serial_mod.o kernel/serial.c run/serial_mod.d; then
  $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall \
      -DNO_DEBUGLOG -Iinclude -MMD -MP -MF run/serial_mod.d \
      -c kernel/serial.c -o run/serial_mod.o
else
  echo "run/serial_mod.o is up to date"
fi

# 6) Compile & link each run/*.c → .elf
for src in run/*.c; do
  [ -f "$src" ] || continue
  base=$(basename "${src%.c}")
  obj="run/${base}.o"
  elf="run/${base}.elf"

  dep="${obj%.o}.d"
  if needs_rebuild "$obj" "$src" "$dep"; then
    echo "Compiling module $src → $obj"
    $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
        -Iinclude -MMD -MP -MF "$dep" -c "$src" -o "$obj"
  else
    echo "Module object $obj is up to date"
  fi

  extra=""
  if [ "$base" = "memtest" ]; then
    # compile memory manager for standalone test
    if needs_rebuild run/memtest_mem.o kernel/mem.c run/memtest_mem.d; then
      $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
          -Iinclude -MMD -MP -MF run/memtest_mem.d -c kernel/mem.c -o run/memtest_mem.o
    fi
    if needs_rebuild run/memtest_memutils.o kernel/memutils.c run/memtest_memutils.d; then
      $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
          -Iinclude -MMD -MP -MF run/memtest_memutils.d -c kernel/memutils.c -o run/memtest_memutils.o
    fi
    extra="run/memtest_mem.o run/memtest_memutils.o"
  fi
  deps=("$obj" run/console_mod.o run/serial_mod.o)
  if [ ${#DEP_OBJS[@]} -gt 0 ]; then
    deps+=(run/linkdep.a)
  fi
  if [ -n "$extra" ]; then
    for e in $extra; do
      deps+=("$e")
    done
  fi
  if should_rebuild "$elf" "${deps[@]}"; then
    echo "Linking $obj + console/serial stubs + linkdep.a → $elf"
    $LD -m $LDARCH -Ttext ${MODULE_BASE} \
        "$obj" run/console_mod.o run/serial_mod.o ${DEP_OBJS:+run/linkdep.a} $extra \
        -o "$elf"
  else
    echo "$elf is up to date"
  fi
done

# build userland modules
USER_OBJS=()
USER_MODULES=()
if [ -d run/userland ]; then
  for src in run/userland/*.c; do
    [ -f "$src" ] || continue
    base=$(basename "${src%.c}")
    obj="run/userland/${base}.o"
    elf="run/userland/${base}.elf"
    dep="${obj%.o}.d"
    if needs_rebuild "$obj" "$src" "$dep"; then
      echo "Compiling userland $src → $obj"
      $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
          -Iinclude -MMD -MP -MF "$dep" -c "$src" -o "$obj"
    else
      echo "Userland object $obj is up to date"
    fi
    deps=("$obj" run/console_mod.o run/serial_mod.o)
    if [ ${#DEP_OBJS[@]} -gt 0 ]; then
      deps+=(run/linkdep.a)
    fi
    extra=""
    if [ "$base" = "03_shell" ]; then
      if needs_rebuild run/userland/shell_mem.o kernel/mem.c run/userland/shell_mem.d; then
        $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
            -Iinclude -MMD -MP -MF run/userland/shell_mem.d -c kernel/mem.c -o run/userland/shell_mem.o
      fi
      if needs_rebuild run/userland/shell_memutils.o kernel/memutils.c run/userland/shell_memutils.d; then
        $CC $MODULE_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -nostdlib -nodefaultlibs \
            -Iinclude -MMD -MP -MF run/userland/shell_memutils.d -c kernel/memutils.c -o run/userland/shell_memutils.o
      fi
      extra="run/userland/shell_mem.o run/userland/shell_memutils.o"
    fi
    if [ -n "$extra" ]; then
      for e in $extra; do
        deps+=("$e")
      done
    fi
    if should_rebuild "$elf" "${deps[@]}"; then
      echo "Linking $obj + console/serial stubs + linkdep.a → $elf"
      $LD -m $LDARCH -Ttext ${MODULE_BASE} \
          "$obj" run/console_mod.o run/serial_mod.o ${DEP_OBJS:+run/linkdep.a} $extra \
          -o "$elf"
    else
      echo "$elf is up to date"
    fi
    USER_MODULES+=( "$elf" )
  done
  for asm in run/userland/*.asm; do
    [ -f "$asm" ] || continue
    bin="run/userland/$(basename "${asm%.asm}.bin")"
    if [ ! -f "$bin" ] || [ "$asm" -nt "$bin" ]; then
      echo "Assembling $asm → $bin"
      $NASM -f bin "$asm" -o "$bin"
    else
      echo "$bin is up to date"
    fi
    USER_MODULES+=( "$bin" )
  done
fi

# 7) Assemble any NASM .asm → .bin modules
for asm in run/*.asm; do
  [ -f "$asm" ] || continue
  bin="run/$(basename "${asm%.asm}.bin")"
  if [ ! -f "$bin" ] || [ "$asm" -nt "$bin" ]; then
    echo "Assembling $asm → $bin"
    $NASM -f bin "$asm" -o "$bin"
  else
    echo "$bin is up to date"
  fi
done

# Skip compilation to .mpy; copy raw .py files instead
for py in run/*.py; do
  [ -f "$py" ] || continue
  : # no compilation needed
done
if [ -d run/userland ]; then
  for py in run/userland/*.py; do
    [ -f "$py" ] || continue
    USER_MODULES+=( "$py" )
  done
fi
if [ -d init/kernel/mgmntshell ]; then
  while IFS= read -r -d '' py; do
    USER_MODULES+=( "${py#./}" )
  done < <(find init/kernel/mgmntshell -maxdepth 1 -type f -name '*.py' -print0)
fi

# Build MicroPython objects for embedding
MP_BUILD=mpbuild
mkdir -p "$MP_BUILD"
MP_SRC="$MP_DIR/examples/embedding/micropython_embed"
MP_OBJS=()
while IFS= read -r -d '' src; do
  obj="$MP_BUILD/$(echo ${src#$MP_SRC/} | tr '/-' '__' | sed 's/\.c$/.o/')"
  dep="${obj%.o}.d"
  if needs_rebuild "$obj" "$src" "$dep"; then
    echo "Compiling Micropython $src → $obj"
    $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -Iinclude -I"$MP_DIR/examples/embedding" -I"$MP_SRC" -I"$MP_SRC/port" \
        -MMD -MP -MF "$dep" -c "$src" -o "$obj"
  else
    echo "Micropython object $obj is up to date"
  fi
  MP_OBJS+=("$obj")
done < <(find "$MP_SRC" -name '*.c' -print0)
if needs_rebuild kernel/micropython.o kernel/micropython.c kernel/micropython.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -Iinclude -I"$MP_DIR/examples/embedding" -I"$MP_SRC" -I"$MP_SRC/port" \
      -MMD -MP -MF kernel/micropython.d -c kernel/micropython.c -o kernel/micropython.o
else
  echo "kernel/micropython.o is up to date"
fi



# 8) Compile & assemble the kernel
echo "Compiling kernel..."
BOOT_ARCH="$ARCH_FLAG"
if [ "$arch_choice" = "3" ]; then
  BOOT_ARCH="-m64"
fi
if needs_rebuild arch/x86/boot.o arch/x86/boot.S ""; then
  $CC $BOOT_ARCH -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -c arch/x86/boot.S   -o arch/x86/boot.o
else
  echo "arch/x86/boot.o is up to date"
fi
if needs_rebuild arch/x86/idt.o arch/x86/idt.S ""; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -c arch/x86/idt.S    -o arch/x86/idt.o
else
  echo "arch/x86/idt.o is up to date"
fi
if needs_rebuild arch/x86/user.o arch/x86/user.S ""; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -c arch/x86/user.S   -o arch/x86/user.o
else
  echo "arch/x86/user.o is up to date"
fi
if needs_rebuild kernel/main.o kernel/main.c kernel/main.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/main.d -c kernel/main.c -o kernel/main.o
else
  echo "kernel/main.o is up to date"
fi
if needs_rebuild kernel/mem.o kernel/mem.c kernel/mem.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/mem.d -c kernel/mem.c -o kernel/mem.o
fi
if needs_rebuild kernel/console.o kernel/console.c kernel/console.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/console.d -c kernel/console.c -o kernel/console.o
fi
if needs_rebuild kernel/serial.o kernel/serial.c kernel/serial.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/serial.d -c kernel/serial.c -o kernel/serial.o
fi
if needs_rebuild kernel/idt.o kernel/idt.c kernel/idt_c.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/idt_c.d -c kernel/idt.c -o kernel/idt.o
fi
if needs_rebuild kernel/panic.o kernel/panic.c kernel/panic.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/panic.d -c kernel/panic.c -o kernel/panic.o
fi
if needs_rebuild kernel/memutils.o kernel/memutils.c kernel/memutils.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/memutils.d -c kernel/memutils.c -o kernel/memutils.o
fi
if needs_rebuild kernel/fs.o kernel/fs.c kernel/fs.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/fs.d -c kernel/fs.c -o kernel/fs.o
fi
if needs_rebuild kernel/script.o kernel/script.c kernel/script.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/script.d -c kernel/script.c -o kernel/script.o
fi
if needs_rebuild kernel/debuglog.o kernel/debuglog.c kernel/debuglog.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/debuglog.d -c kernel/debuglog.c -o kernel/debuglog.o
fi
if needs_rebuild kernel/syscall.o kernel/syscall.c kernel/syscall.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/syscall.d -c kernel/syscall.c -o kernel/syscall.o
fi
if needs_rebuild kernel/modexec.o kernel/modexec.c kernel/modexec.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/modexec.d -c kernel/modexec.c -o kernel/modexec.o
fi
if needs_rebuild kernel/mpy_loader.o kernel/mpy_loader.c kernel/mpy_loader.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/mpy_loader.d -c kernel/mpy_loader.c -o kernel/mpy_loader.o
fi
if needs_rebuild kernel/mpy_modules.o kernel/mpy_modules.c kernel/mpy_modules.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/mpy_modules.d -c kernel/mpy_modules.c -o kernel/mpy_modules.o
fi
if needs_rebuild kernel/vga_draw.o kernel/vga_draw.c kernel/vga_draw.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/vga_draw.d -c kernel/vga_draw.c -o kernel/vga_draw.o
fi
if needs_rebuild kernel/framebuffer.o kernel/framebuffer.c kernel/framebuffer.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/framebuffer.d -c kernel/framebuffer.c -o kernel/framebuffer.o
fi
if needs_rebuild kernel/io.o linkdep/io.c kernel/io.d; then
  $CC $ARCH_FLAG -std=gnu99 -ffreestanding -O2 $STACK_FLAGS -fcf-protection=none -Wall -U__linux__ -Iinclude \
      -MMD -MP -MF kernel/io.d -c linkdep/io.c -o kernel/io.o
fi
# 9) Link into flat kernel.bin
KERNEL_OBJECTS=(
  arch/x86/boot.o arch/x86/idt.o arch/x86/user.o
  kernel/main.o kernel/mem.o kernel/console.o kernel/serial.o
  kernel/idt.o kernel/panic.o kernel/memutils.o kernel/fs.o kernel/script.o
  kernel/debuglog.o kernel/syscall.o kernel/micropython.o kernel/mpy_loader.o
  kernel/mpy_modules.o kernel/modexec.o kernel/vga_draw.o kernel/framebuffer.o kernel/io.o
)
KERNEL_LINK_DEPS=("${KERNEL_OBJECTS[@]}" "${MP_OBJS[@]}" linker.ld)
if should_rebuild kernel.bin "${KERNEL_LINK_DEPS[@]}"; then
  echo "Linking kernel.bin..."
  $LD -m $LDARCH -T linker.ld \
      "${KERNEL_OBJECTS[@]}" \
      "${MP_OBJS[@]}" \
      -o kernel.bin
else
  echo "kernel.bin is up to date"
fi

# 10) Prepare ISO tree
mkdir -p isodir/boot/grub
copy_if_different kernel.bin isodir/boot/kernel.bin
ISO_DEPS=(isodir/boot/kernel.bin)

# 11) Copy modules into ISO
MODULES=()
for m in run/*.{bin,elf,ts,py,mpy}; do
  [ -f "$m" ] || continue
  bn=$(basename "$m")
  copy_if_different "$m" "isodir/boot/$bn"
  MODULES+=( "$bn" )
  ISO_DEPS+=("isodir/boot/$bn")
done
USER_MODULES_BN=()
for m in "${USER_MODULES[@]}"; do
  [ -f "$m" ] || continue
  case "$m" in
    init/kernel/mgmntshell/*)
      dest="$m"
      ;;
    *)
      dest=$(basename "$m")
      ;;
  esac
  mkdir -p "isodir/boot/$(dirname "$dest")"
  copy_if_different "$m" "isodir/boot/$dest"
  USER_MODULES_BN+=( "$dest" )
  ISO_DEPS+=("isodir/boot/$dest")
done

# include init script if present
INIT_SCRIPT="init/kernel/init.py"
INIT_ELF="init/kernel/init.elf"
if [ -f "$INIT_SCRIPT" ]; then
  mkdir -p isodir/boot/init/kernel
  copy_if_different "$INIT_SCRIPT" isodir/boot/init/kernel/init.py
  MODULES+=( "init/kernel/init.py" )
  ISO_DEPS+=(isodir/boot/init/kernel/init.py)
elif [ -f "$INIT_ELF" ]; then
  mkdir -p isodir/boot/init/kernel
  copy_if_different "$INIT_ELF" isodir/boot/init/kernel/init.elf
  MODULES+=( "init/kernel/init.elf" )
  ISO_DEPS+=(isodir/boot/init/kernel/init.elf)
fi

# 12) Generate grub.cfg
tmp_cfg=$(mktemp)
{
  cat <<'CFG'
set timeout=5
set default=0
terminal_output console

menuentry "NO VGA" {
  insmod all_video
  insmod gfxterm
  set gfxmode=640x480x32
  set gfxpayload=keep
  terminal_output gfxterm
  multiboot /boot/kernel.bin novgacon
CFG
  for mod in "${MODULES[@]}"; do
    printf '  module /boot/%s %s\n' "$mod" "$mod"
  done
  cat <<'CFG'
  boot
}

menuentry "ExoCore Alpha" {
  multiboot /boot/kernel.bin
CFG
  for mod in "${MODULES[@]}"; do
    printf '  module /boot/%s %s\n' "$mod" "$mod"
  done
  cat <<'CFG'
  boot
}

menuentry "ExoCore-Kernel (Debug)" {
  multiboot /boot/kernel.bin debug
CFG
  for mod in "${MODULES[@]}"; do
    printf '  module /boot/%s %s\n' "$mod" "$mod"
  done
  cat <<'CFG'
  boot
}

menuentry "ExoCore-Management-shell (alpha)" {
  multiboot /boot/kernel.bin userland
CFG
  for mod in "${MODULES[@]}"; do
    printf '  module /boot/%s %s\n' "$mod" "$mod"
  done
  for mod in "${USER_MODULES_BN[@]}"; do
    printf '  module /boot/%s userland /boot/%s\n' "$mod" "$mod"
  done
  cat <<'CFG'
  boot
}
CFG
} > "$tmp_cfg"
if [ -f isodir/boot/grub/grub.cfg ] && cmp -s "$tmp_cfg" isodir/boot/grub/grub.cfg; then
  rm -f "$tmp_cfg"
else
  mv "$tmp_cfg" isodir/boot/grub/grub.cfg
fi
ISO_DEPS+=(isodir/boot/grub/grub.cfg)

# 13) Build the ISO
if should_rebuild exocore.iso "${ISO_DEPS[@]}"; then
  echo "Building ISO (modules: ${MODULES[*]} userland: ${USER_MODULES_BN[*]})..."
  $GRUB -o exocore.iso isodir
else
  echo "exocore.iso is up to date"
fi

refresh_iso_cache

# 14) Run in QEMU if requested
  if [ "$1" = "run" ]; then
    echo "Booting in QEMU…"
    if [ "$2" = "nographic" ]; then
      $QEMU -cdrom exocore.iso \
           -boot order=d \
           -serial stdio \
           -monitor none \
           -no-reboot \
           -display none \
           ${QEMU_EXTRA:+$QEMU_EXTRA}
    else
      $QEMU -cdrom exocore.iso \
           -boot order=d \
           -serial stdio \
           -monitor none \
           -no-reboot \
           ${QEMU_EXTRA:+$QEMU_EXTRA}
    fi
  else
    echo "Done, use './build.sh run [nographic]' to build & boot"
  fi
