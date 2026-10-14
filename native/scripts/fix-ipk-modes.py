#!/usr/bin/env python3
"""Rewrite an IPK so nothing in it is group- or world-writable.

ares-package stores every entry as 0777 (directories, executables) or 0666
(files), whatever the staged modes were. The TV installs what the archive
says, so the app's files would be writable by any process on the TV. This
rewrites both tarballs inside the IPK: directories 0755, executables 0755,
everything else 0644. Entry order, names, owners and contents are kept.

    fix-ipk-modes.py path/to/app.ipk
"""
import gzip
import io
import sys
import tarfile


def read_ar(data):
    if not data.startswith(b"!<arch>\n"):
        raise SystemExit("not an ar archive")
    pos, members = 8, []
    while pos < len(data):
        header = data[pos:pos + 60]
        size = int(header[48:58].decode().strip())
        members.append((header, data[pos + 60:pos + 60 + size]))
        pos += 60 + size + (size % 2)
    return members


def write_ar(members):
    out = bytearray(b"!<arch>\n")
    for header, body in members:
        header = header[:48] + str(len(body)).ljust(10).encode() + header[58:]
        out += header + body
        if len(body) % 2:
            out += b"\n"
    return bytes(out)


def fix_tar(blob):
    src = tarfile.open(fileobj=io.BytesIO(blob), mode="r:gz")
    raw = io.BytesIO()
    dst = tarfile.open(fileobj=raw, mode="w", format=src.format)
    for info in src.getmembers():
        if info.isdir():
            info.mode = 0o755
        elif info.isfile():
            info.mode = 0o755 if info.mode & 0o100 else 0o644
        dst.addfile(info, src.extractfile(info) if info.isfile() else None)
    dst.close()
    return gzip.compress(raw.getvalue(), mtime=0)


def main(path):
    with open(path, "rb") as f:
        members = read_ar(f.read())
    fixed = []
    for header, body in members:
        name = header[:16].decode().strip().rstrip("/")
        fixed.append((header, fix_tar(body) if name.endswith(".tar.gz") else body))
    with open(path, "wb") as f:
        f.write(write_ar(fixed))


if __name__ == "__main__":
    main(sys.argv[1])
