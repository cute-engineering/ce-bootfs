#include <std/mem.pat>
#include <std/io.pat>

struct Header {
    u32 magic;
    u32 dirlen;
    u32 reserved;
    u32 reserved2;
};

fn formatDirent(auto var) {
  return std::format("{} ({} bytes)", var.name, var.datalen);
};

struct Dirent {
    u32 namelen;
    u32 datalen;
    u32 dataoff;
    char name[namelen - 1];
    char nul;
    std::mem::AlignTo<4>;
    char data[datalen] @ dataoff;
} [[format("formatDirent")]];

struct Bootfs {
    Header header;
    Dirent dirent[while(!std::mem::reached(header.dirlen + sizeof(Header)))];
};

Bootfs bootfs @ 0x0;
