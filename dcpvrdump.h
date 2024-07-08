//
// Created by . on 7/8/24.
//

#ifndef DCPVRDUMPS_DCPVRDUMP_H
#define DCPVRDUMPS_DCPVRDUMP_H

#include <stdint.h>

/*
 * Versions = 0 = first draft release
 */

/*
Game Info -must be first block if present (just like, name of game, name of first CDROM file, frame # if available, that kinda thing)
Arbitrary Message - must be second block if present
TA FIFO block
Register Update
STARTRENDER
VRAM current
 */

// No single blocks bigger than 16mb allowed
#define ERROR_ON_BLOCKS_BIGGER_THAN (16 * 1024 * 1024)

enum PVD_magic_numbers {
    PVD_MN_file = 0xe8ba0e52,
    PVD_MN_TA_FIFO_block = 0xadd685f1,
    PVD_MN_game_info = 0x78d6a805,
    PVD_MN_register_update = 0x5930a419,
    PVD_MN_VRAM = 0xd7be4263,
    PVD_MN_arbitrary_message = 0xa1e831d2
};

struct PVD_header {
    uint32_t section_size;
    uint32_t offset_from_start;

    union {
        struct PVD_file_header {
            uint32_t total_size;
            uint32_t magic_number;
            uint32_t version;
            uint32_t offset_to_first_block;
        } file __attribute__((packed));

        struct PVD_section_header {
            uint32_t section_size;          // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0
        } section __attribute__((packed));

        struct PVD_game_info_header {
            uint32_t section_size;          // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0

            int64_t frame;      // frame # if available, -1 otherwise
            uint32_t name_length; // Length of name, 0-49
            char name[50];      // null-terminated. 49 chars + 1
            uint32_t filename_length; // length of filename, 0-49
            char filename[50];  // null-terminated, 49 chars + 1
        } game_info __attribute__((packed));

        struct PVD_arbitrary_message_header {
            uint32_t section_size;          // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0

            uint32_t size_of_message; // We should put some reasonable limit on this like 10k
        } arbitrary_message __attribute__((packed));

        struct PVD_register_update_header {
            uint32_t section_size;          // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0

            uint32_t num_register_updates; // Number of register updates we contain
        } register_update __attribute__((packed));

        struct PVD_vram_header {
            uint32_t section_size;  // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0

            uint32_t start_offset;  // Starting offset in VRAM this is
            uint32_t length;        // Length of VRAM this is
        } vram __attribute__((packed));

        struct PVD_fifo_header {
            uint32_t section_size;  // includes 12 bytes of the section header
            uint32_t magic_number;
            uint32_t version;       // should just always default to 0
            uint32_t flags;         // blank for now

            uint32_t length;        // Length of data in the FIFO buffer
        } fifo;
    } m __attribute__((packed));
};


struct PVD_reader_writer {
    struct PVD_header file_header;
    uint8_t *buffer;
    uint8_t *ptr;
    uint32_t buffer_size;
    int32_t errored; // 0 for no error
};

struct PVD_register_update {
    uint32_t addr;
    uint32_t val;
    uint32_t sz; // 1 = 8bit, 2 = 16bit, 4 = 32bit
} __attribute__((packed));


// Return 0 for success, -1 for error I guess

// Must call this first
struct PVD_reader_writer *PVD_start_read_file(char *inbuffer, uint32_t inbuffer_len);
struct PVD_reader_writer *PVD_start_write_file(char *outbuffer, uint32_t outbuffer_len);

struct PVD_header PVD_read_section_header(struct PVD_reader_writer *s);

// Returns # bytes read
int PVD_read_FIFO_VRAM_arbitrary_message(struct PVD_reader_writer *s, struct PVD_header *h, char *outbuffer, uint32_t outbuffer_len);
struct PVD_register_update PVD_read_single_register_update(struct PVD_reader_writer *s, struct PVD_header *h);

void PVD_write_game_info(struct PVD_reader_writer *s, const char*name, const char*filename, uint64_t frame_number);
void PVD_write_arbitrary_message(struct PVD_reader_writer *s, char *inbuf, uint32_t inbuf_len);
void PVD_write_register_update(struct PVD_reader_writer *s, uint32_t num_updates, struct PVD_register_update *updates);
void PVD_write_VRAM(struct PVD_reader_writer *s, uint32_t offset, uint32_t len, char *inbuffer);
void PVD_write_FIFO(struct PVD_reader_writer *s, uint32_t offset, char *inbuffer, uint32_t inbuffer_len);

void PVD_delete_reader_writer(struct PVD_reader_writer *s);

#endif //DCPVRDUMPS_DCPVRDUMP_H
