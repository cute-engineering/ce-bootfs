# BootFS File Format Specification

Based on [Fuchsia Documentation](https://fuchsia.dev/fuchsia-src/concepts/process/userboot#bootfs)

## Overview

BootFS is a simple file container format used for packaging files in memory. It consists of a fixed-size header, a directory region describing all files, and a file data region containing the payloads. All multi-byte integers are **little-endian 32-bit unsigned**.

---

## Constants

* **Page size:** 4096 bytes
* **Magic value:** `0xA56D3FF9` (decimal `2775400441`)
* **Maximum name length:** 256 bytes (including trailing NUL)

---

## File Layout

```
+---------------------------+ 0x0000
| BootFS Header (16 bytes)  |
+---------------------------+ 0x0010
| Directory Region          | dirsize bytes
|  - Directory Entry [0]    |
|  - Directory Entry [1]    |
|  - ...
+---------------------------+ 0x0010 + dirsize
| File Data Region          |
|  - File payloads (page-aligned)
+---------------------------+
```

---

## BootFS Header

The header occupies the first 16 bytes.

| Field       | Type | Description                                                               |
| ----------- | ---- | ------------------------------------------------------------------------- |
| `magic`     | u32  | Must equal `0xA56D3FF9`.                                                  |
| `dirsize`   | u32  | Total size of the directory region in bytes. Does not include the header. |
| `reserved0` | u32  | Reserved, must be zero.                                                   |
| `reserved1` | u32  | Reserved, must be zero.                                                   |

---

## Directory Region

The directory region is a sequence of variable-length entries. Parsing continues until `dirsize` bytes are consumed.

### Directory Entry

Each entry starts with a 12-byte fixed header, followed by the filename, then padding.

| Field      | Type | Description                                                                                |
| ---------- | ---- | ------------------------------------------------------------------------------------------ |
| `name_len` | u32  | Length of the filename in bytes, including the trailing NUL. Must be 1 ≤ `name_len` ≤ 256. |
| `data_len` | u32  | Size of the file payload in bytes.                                                         |
| `data_off` | u32  | File data offset (see below).                                                              |

After this struct:

* **Filename:** `name_len` bytes, UTF-8, NUL-terminated.
* **Padding:** The total size of the entry (struct + filename) is rounded up to a multiple of 4 bytes.

**Directory entry size formula:**

```
entry_size = align4(12 + name_len)
```

---

## File Data Region

File contents are stored in the data region after the directory region.

* Each file’s payload begins at an offset computed as:

```
file_offset = align_page(data_off)
align_page(x) = (x + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1)
```

* Payload length = `data_len` bytes.
* Offsets are relative to the start of the BootFS blob.

---

## Validation Rules

An implementation must reject a BootFS image if:

1. `magic` does not equal `0xA56D3FF9`.
2. `dirsize` is smaller than the minimum directory entry size (12 bytes).
3. Any directory entry extends beyond the declared `dirsize`.
4. `name_len` is outside the range [1, 256].
5. The filename is not NUL-terminated or is invalid UTF-8.
6. File offsets or sizes would require reading beyond the container size.

