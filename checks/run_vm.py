"""Boot each Talios regression on fresh, disposable Bochs media."""
import argparse
import os
from pathlib import Path
import re
import selectors
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
PAGE = 4096


def populate(path, fat_bits, files, size, boot_sector=None):
    """Populate the root of a freshly formatted FAT12 or FAT32 test volume."""
    with path.open("wb") as image:
        image.truncate(size)
    subprocess.run(["mkfs.fat", "-F", str(fat_bits), "-s", "1", str(path)],
                   check=True, stdout=subprocess.DEVNULL)
    with path.open("r+b") as image:
        boot = image.read(512)
        sector, = struct.unpack_from("<H", boot, 11)
        cluster_size = sector * boot[13]
        reserved, = struct.unpack_from("<H", boot, 14)
        copies = boot[16]
        roots, = struct.unpack_from("<H", boot, 17)
        is_floppy = fat_bits == 12
        fat_sectors, = struct.unpack_from("<H" if is_floppy else "<I", boot, 22 if is_floppy else 36)
        root_start = (reserved + copies * fat_sectors) * sector
        data_start = root_start + ((roots * 32 + sector - 1) // sector) * sector
        root_cluster = 0 if is_floppy else struct.unpack_from("<I", boot, 44)[0]
        root_offset = root_start if is_floppy else data_start + (root_cluster - 2) * cluster_size
        next_cluster = 2 if is_floppy else root_cluster + 1
        image.seek(reserved * sector)
        fat = bytearray(image.read(fat_sectors * sector))
        locations = {}
        for slot, (name, payload) in enumerate(files):
            count = (len(payload) + cluster_size - 1) // cluster_size
            # Fragment the entire file used for offset and writeback tests.
            step = 2 if name == b"VMCHECK DAT" else 1
            chain = list(range(next_cluster, next_cluster + count * step, step))
            next_cluster += count * step
            offsets = []
            for i, cluster in enumerate(chain):
                following = chain[i + 1] if i + 1 < count else (1 << fat_bits) - 1
                if is_floppy:
                    idx = cluster + cluster // 2
                    old, = struct.unpack_from("<H", fat, idx)
                    val = ((old & 15) | (following << 4)) if cluster & 1 else ((old & 0xf000) | following)
                    struct.pack_into("<H", fat, idx, val)
                else:
                    struct.pack_into("<I", fat, cluster * 4, following & 0x0fffffff)
                offset = data_start + (cluster - 2) * cluster_size
                if offset + cluster_size > size:
                    raise ValueError("test volume is too small")
                offsets.append(offset)
                image.seek(offset)
                image.write(payload[i * cluster_size:(i + 1) * cluster_size])
            locations[name] = offsets
            entry = bytearray(32)
            entry[:11] = name
            entry[11] = 0x20
            first = chain[0] if chain else 0
            struct.pack_into("<H", entry, 20, first >> 16)
            struct.pack_into("<H", entry, 26, first & 0xffff)
            struct.pack_into("<I", entry, 28, len(payload))
            image.seek(root_offset + slot * 32)
            image.write(entry)
        for i in range(copies):
            image.seek((reserved + i * fat_sectors) * sector)
            image.write(fat)
        if boot_sector:
            image.seek(0)
            image.write(boot_sector[:512])
        return locations, cluster_size


def boot(work, binaries, init, marker, cpus, bochs):
    work.mkdir()
    populate(work / "boot.img", 12,
             [(b"LOADER  BIN", (binaries / "loader.bin").read_bytes()),
              (b"KERNEL  BIN", (binaries / "kernel.bin").read_bytes())],
             1440 * 1024, (binaries / "boot.bin").read_bytes())
    fixture = bytes([1]) * PAGE + bytes([2]) * PAGE + bytes([3]) * 37
    disk = work / "disk.img"
    files = [(b"INIT    BIN", (binaries / init).read_bytes()), (b"KEYBOARDDEV", b""),
             (b"VMCHECK DAT", fixture), (b"VMTRUNC DAT", b"Test")]
    if init == "schedcheck.bin":
        files.append((b"CPUS    DAT", bytes([cpus])))
    locations, cluster_size = populate(disk, 32, files, 128 * 1024 * 1024)
    config = (ROOT / "bochsrc.floppy").read_text()
    for old, new in [
        ("display_library: x", "display_library: nogui"),
        ("bin/boot.img", "boot.img"), ("bin/fat32disk.img", "disk.img"),
        ("host=2048, guest=2048", "host=128, guest=128"), ("megs: 2048", "megs: 128"),
        ("port_e9_hack: enabled=0", "port_e9_hack: enabled=1"),
        ("speaker: enabled=1", "speaker: enabled=0"), ("\nlog: -\n", "\nlog: bochs.log\n"),
        ("panic: action=ask", "panic: action=fatal")]:
        config = config.replace(old, new)
    (work / "bochsrc").write_text(config)
    topology = f"1:{cpus}:1" if cpus <= 8 else f"{cpus}:1:1"
    guest = subprocess.Popen(["stdbuf", "-o0", bochs, "-q", "-f", "bochsrc",
                              f"cpu: count={topology}"], cwd=work,
                             stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
    output = bytearray()
    try:
        with selectors.DefaultSelector() as reader:
            reader.register(guest.stdout, selectors.EVENT_READ)
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                if not reader.select(1):
                    if guest.poll() is not None:
                        break
                    continue
                data = os.read(guest.stdout.fileno(), 8192)
                if not data:
                    break
                output.extend(data)
                if b"do_pageFault(14)" in output or re.search(rb"(?:VM|SCHED) FAIL[^\n]*\n", output):
                    break
                if marker in output:
                    break
    finally:
        guest.terminate()
        try:
            guest.wait(3)
        except subprocess.TimeoutExpired:
            guest.kill()
            guest.wait()
    online = f"SMP online {cpus}/{cpus} CPUs".encode()
    if marker not in output or online not in output or re.search(rb"(?:VM|SCHED) FAIL", output):
        log = work / "bochs.log"
        detail = log.read_bytes()[-1000:] if log.exists() else b""
        raise RuntimeError(f"{init}, {cpus} CPUs\n" + (output[-4000:] + detail).decode(errors="replace"))
    if init == "vmcheck.bin":
        with disk.open("rb") as image:
            image.seek(locations[b"VMCHECK DAT"][PAGE // cluster_size])
            if image.read(4) != bytes([5, 8, 9, 2]):
                raise RuntimeError("FAT32 writeback differs on the host image")
    if init == "schedcheck.bin":
        if f"SCHED concurrent tasks {cpus}\n".encode() not in output:
            raise RuntimeError("The requested CPUs did not own distinct tasks concurrently")
        if cpus > 1 and b"SCHED remote TLB invalidation\n" not in output:
            raise RuntimeError("No remote CPU invalidated an active address space")
        if f"SCHED user CPUs {cpus},".encode() not in output:
            raise RuntimeError("User-mode APIC samples did not cover all configured CPUs\n" +
                               output[-1200:].decode(errors="replace"))
        with disk.open("rb") as image:
            image.seek(locations[b"VMCHECK DAT"][0])
            if image.read(2) != b"AZ":
                raise RuntimeError("Second shared-page write did not reach the disk image")
        for line in output.splitlines():
            if line.startswith(b"SCHED ") and line != b"SCHED PASS":
                print(f"[{cpus} CPUs] {line.decode()}", flush=True)
    print(f"[{cpus} CPUs] {marker.decode().strip()}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sched", action="store_true", help="also run scheduler and idle-wakeup checks")
    parser.add_argument("--cpus", type=int, nargs="+", default=[1], choices=range(1, 17))
    parser.add_argument("--bochs", default="bochs")
    args = parser.parse_args()
    bochs = str(Path(args.bochs).resolve()) if os.sep in args.bochs else args.bochs
    with tempfile.TemporaryDirectory(prefix="talios-check-") as temp:
        work = Path(temp)
        binaries = work / "bin"
        flags = "EXTRA_CFLAGS=-DTALIOS_DEBUG_E9"
        if args.sched:
            flags += " -DTALIOS_SCHED_CHECK"
        build = subprocess.run(["make", "-s", "-j4", f"OBJDIR={work / 'obj'}", f"BINDIR={binaries}",
                                flags, "all", "vmcheck"] + (["schedcheck"] if args.sched else []), cwd=ROOT,
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if build.returncode:
            raise RuntimeError(build.stdout.decode(errors="replace"))
        for n in dict.fromkeys(args.cpus):
            boot(work / f"shell-{n}", binaries, "init.bin", b"[SHELL]#:", n, bochs)
            boot(work / f"vm-{n}", binaries, "vmcheck.bin", b"VM PASS\n", n, bochs)
            if args.sched:
                boot(work / f"sched-{n}", binaries, "schedcheck.bin", b"SCHED PASS\n", n, bochs)
