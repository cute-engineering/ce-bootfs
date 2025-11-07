#pragma endian little
#pragma magic [F9 3F 6D A5] @ 0x0

import std.mem;
import std.io;

#ifdef __IMHEX__
import hex.core;
#endif

struct Header {
    u32 magic;
    u32 dirlen;
    u32 reserved;
    u32 reserved2;
};

fn formatDirent(auto var) {
    #ifdef __IMHEX__
        hex::core::add_virtual_file(var.name, var.data);
    #endif
    return std::format("{} ({} bytes)", var.name, var.datalen);
};

struct Dirent {
    u32 namelen;
    u32 datalen;
    u32 dataoff;
    char name[namelen - 1];
    char nul;
    std::mem::AlignTo<4>;
    u8 data[datalen] @ dataoff;
} [[format("formatDirent")]];

struct Bootfs {
    Header header;
    Dirent dirent[while(!std::mem::reached(header.dirlen + sizeof(Header)))];
};

Bootfs bootfs @ 0x0;