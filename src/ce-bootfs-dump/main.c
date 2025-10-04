#include <ce-bootfs/bootfs.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

void hexdump(void* data, size_t size) {
    uint8_t* bytes = (uint8_t*)data;
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            printf("%08zx  ", i);
        }
        printf("%02x ", bytes[i]);
        if (i % 16 == 15 || i == size - 1) {
            for (size_t j = i % 16 + 1; j < 16; j++) {
                printf("   ");
            }
            printf(" |");
            for (size_t j = i - (i % 16); j <= i; j++) {
                if (bytes[j] >= 32 && bytes[j] <= 126) {
                    printf("%c", bytes[j]);
                } else {
                    printf(".");
                }
            }
            printf("|\n");
        }
    }
}

int main(int argc, char** argv) {
    // dump bootfs image to stdout
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <bootfs image>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    void* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    bootfs_header_t* header = (bootfs_header_t*)data;
    if (header->magic != BOOTFS_MAGIC) {
        fprintf(stderr, "Invalid bootfs image\n");
        return 1;
    }

    bootfs_iter_t iter = bootfs_iter(header);
    printf("BootFS Image:\n");
    printf("Magic: 0x%08x\n", header->magic);
    printf("Directory Length: %u bytes\n", header->dirlen);
    printf("Files:\n");
    for (bootfs_dirent_t* dirent = bootfs_next(&iter); dirent != NULL; dirent = bootfs_next(&iter)) {
        printf("File: %s, Size: %u bytes, Offset: %u\n", dirent->name, dirent->length, dirent->offset);
        hexdump((uint8_t*)data + dirent->offset, dirent->length);
        printf("\n");
    }

    munmap(data, st.st_size);
    close(fd);
    return 0;
}
