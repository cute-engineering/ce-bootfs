from pathlib import Path
from dataclasses import dataclass
from typing import Self
from struct import pack
import argparse

MAGIC = 0xA56D3FF9
BLOCK = 4096


def alignUp(value: int, alignment: int) -> int:
    return (value + alignment - 1) & ~(alignment - 1)


@dataclass
class Entry:
    name: str
    data: bytes
    offset: int = 0

    @classmethod
    def load(cls: type[Self], path: Path, name: str) -> Self:
        return cls(name, path.open("rb").read())

    def encodedSize(self) -> int:
        return alignUp(
            4  # name_len
            + 4  # data_len
            + 4  # data_off
            + len(self.name.encode())
            + 1,
            4,
        )

    def encode(self) -> bytes:
        nameu8 = self.name.encode()

        dat = (
            pack("<III", len(nameu8) + 1, len(self.data), self.offset) + nameu8 + b"\0"
        )
        pad = b"\0" * (self.encodedSize() - len(dat))
        return dat + pad


@dataclass
class BootFS:
    entries: list[Entry]

    @classmethod
    def create(cls: type[Self], paths: list[tuple[Path, str]]) -> Self:
        return cls([Entry.load(p, n) for p, n in paths])

    def encodeHeader(self) -> bytes:
        dirsize = sum(e.encodedSize() for e in self.entries)
        return pack("<IIII", MAGIC, dirsize, 0, 0)

    def encodedHeaderSize(self) -> int:
        return 16  # 4 * 4

    def encodeDirectory(self) -> bytes:
        return b"".join(e.encode() for e in self.entries)

    def fileDataOffset(self) -> int:
        return alignUp(
            self.encodedHeaderSize() + sum(e.encodedSize() for e in self.entries), BLOCK
        )

    def headerAndDirectorySize(self) -> int:
        return self.encodedHeaderSize() + sum(e.encodedSize() for e in self.entries)

    def computeOffsets(self) -> None:
        offset = self.fileDataOffset()
        for entry in self.entries:
            entry.offset = offset
            offset += alignUp(len(entry.data), BLOCK)

    def encodeFileData(self) -> bytes:
        data = b""
        for entry in self.entries:
            data += entry.data
            data += b"\0" * (alignUp(len(entry.data), BLOCK) - len(entry.data))
        return data

    def encode(self) -> bytes:
        self.computeOffsets()
        return (
            self.encodeHeader()
            + self.encodeDirectory()
            + b"\0" * (self.fileDataOffset() - self.headerAndDirectorySize())
            + self.encodeFileData()
        )

    def save(self, path: Path) -> None:
        path.open("wb").write(self.encode())


if "__main__" == __name__:
    parser = argparse.ArgumentParser(description="Create a bootfs image")
    parser.add_argument("output", type=Path, help="Output bootfs image path")
    parser.add_argument(
        "inputs",
        type=str,
        nargs="*",
        help="Input files in the format <path>:<name>",
    )
    parser.add_argument(
        "--dir",
        type=Path,
        help="Directory to load all files from recursively, using filenames as names",
        default=None,
    )
    args = parser.parse_args()

    if args.dir:
        args.inputs = [
            f"{f}:{f.relative_to(args.dir)}" for f in args.dir.rglob("*") if f.is_file()
        ]

    inputs = []
    for inp in args.inputs:
        path, name = inp.split(":", 1)
        inputs.append((Path(path), name))

    bootfs = BootFS.create(inputs)
    bootfs.save(args.output)
