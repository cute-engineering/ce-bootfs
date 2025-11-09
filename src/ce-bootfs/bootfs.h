#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define BOOTFS_MAGIC (0xA56D3FF9)
#define BOOTFS_BLOCK (4096)
#define BOOTFS_NAME_MAX (256)
#define BOOTFS_DIRENT_ALIGN (4)

typedef struct {
    uint32_t magic;
    uint32_t dirlen;
    uint32_t reserved;
    uint32_t reserved2;
} bootfs_header_t;

typedef struct {
    uint32_t namelen;
    uint32_t length;
    uint32_t offset;
    char name[];
} bootfs_dirent_t;

typedef struct {
    bootfs_dirent_t* dirent;
    uint8_t* data;
    size_t length;
} bootfs_file_t;

typedef struct {
    bootfs_dirent_t* _begin;
    bootfs_dirent_t* _end;
} bootfs_iter_t;

static inline size_t bootfs_align(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

static inline bootfs_iter_t bootfs_iter(bootfs_header_t const* header) {
    bootfs_dirent_t* begin = (bootfs_dirent_t*)((uint8_t*)header + sizeof(bootfs_header_t));
    bootfs_dirent_t* end = (bootfs_dirent_t*)((uint8_t*)begin + header->dirlen);
    return (bootfs_iter_t){begin, end};
}

static inline bootfs_dirent_t* bootfs_next(bootfs_iter_t* iter) {
    if (iter->_begin >= iter->_end) {
        return NULL;
    }

    bootfs_dirent_t* current = iter->_begin;
    size_t entry_size = sizeof(bootfs_dirent_t) + bootfs_align(current->namelen, BOOTFS_DIRENT_ALIGN);
    iter->_begin = (bootfs_dirent_t*)((uint8_t*)iter->_begin + entry_size);
    return current;
}

static inline int bootfs_memcmp(void const* a, void const* b, size_t n) {
    uint8_t const* p1 = (uint8_t const*)a;
    uint8_t const* p2 = (uint8_t const*)b;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}

static inline bootfs_dirent_t* bootfs_find(bootfs_header_t* header, char const* name, size_t namelen) {
    bootfs_iter_t iter = bootfs_iter(header);
    bootfs_dirent_t* entry;
    while ((entry = bootfs_next(&iter)) != NULL) {
        if (entry->namelen == namelen && bootfs_memcmp(entry->name, name, namelen) == 0) {
            return entry;
        }
    }
    return NULL;
}

static inline bootfs_file_t bootfs_open(bootfs_header_t* header, char const* name, size_t namelen) {
    bootfs_dirent_t* entry = bootfs_find(header, name, namelen);
    if (!entry) {
        return (bootfs_file_t){NULL, NULL, 0};
    }
    uint8_t* data = (uint8_t*)header + entry->offset;
    return (bootfs_file_t){entry, data, entry->length};
}

static inline size_t bootfs_readat(bootfs_file_t* file, void* buffer, size_t size, size_t offset) {
    if (offset >= file->length) {
        return 0;
    }
    if (offset + size > file->length) {
        size = file->length - offset;
    }
    for (size_t i = 0; i < size; i++) {
        ((uint8_t*)buffer)[i] = file->data[offset + i];
    }
    return size;
}

#ifdef __cplusplus
}
#endif
