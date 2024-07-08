//
// Created by . on 7/8/24.
//

#include <stdlib.h>
#include <stdio.h>

#include "dcpvrdump.h"


#define S_TELL    (s->ptr - s->buffer)
#define S_BYTES_LEFT    (s->buffer_size - (s->ptr - s->buffer))

#define dbg_printf(...) printf(__VA_ARGS__)

struct PVD_reader_writer *PVD_start_read_file(char *inbuffer, uint32_t inbuffer_len)
{
    if (inbuffer == NULL) {
        dbg_printf("\nPassed NULL pointer to PVD_start_read_file!");
        return NULL;
    }

    struct PVD_reader_writer *s = (struct PVD_reader_writer *)malloc(sizeof(struct PVD_reader_writer));
    s->buffer = (uint8_t *)inbuffer;
    s->ptr = (uint8_t *)inbuffer;
    s->buffer_size = inbuffer_len;
    s->errored = 0;

    if (S_BYTES_LEFT < sizeof(s->file_header.m.file)) {
        dbg_printf("\nExpected at least %li bytes free, have %ld!", sizeof(struct PVD_file_header), S_BYTES_LEFT);
        free(s);
        return NULL;
    }

    s->file_header.m.file = *((struct PVD_file_header *)s->ptr);

    if (s->file_header.m.file.magic_number != PVD_MN_file) {
        dbg_printf("\nFound bad magic number! %08x", s->file_header.m.file.magic_number);
        free(s);
        return NULL;
    }

    if (s->file_header.m.file.total_size < inbuffer_len) {
        dbg_printf("\nHeader says file is %d bytes, but input buffer is only %d!", s->file_header.m.file.total_size, inbuffer_len);
        free(s);
        return NULL;
    }

    s->ptr += sizeof(struct PVD_file_header);

    return s;
}

struct PVD_reader_writer *PVD_start_write_file(char *outbuffer, uint32_t outbuffer_len)
{
    if (outbuffer == NULL) {
        dbg_printf("\nPassed null buffer!");
        return NULL;
    }
}


#define CHECK_LEFT(x, rs, rv) {\
    if (S_BYTES_LEFT < (x)) {\
        dbg_printf("\n%d bytes needed, %d had for %s", (uint32_t)(x), (uint32_t)S_BYTES_LEFT, rs); \
        s->errored = 1;\
        rv;\
       }; }

struct PVD_header PVD_read_section_header(struct PVD_reader_writer *s)
{
    struct PVD_header h;
    h.section_size = 0;
    h.offset_from_start = S_TELL;
    if (S_BYTES_LEFT < sizeof(struct PVD_section_header)) {
        dbg_printf("\nNot enough bytes left! Left:%ld  needed:%ld", S_BYTES_LEFT, sizeof(struct PVD_section_header));
        s->errored = 1;
        return h;
    }

    struct PVD_section_header sh = *(struct PVD_section_header *)s->ptr;

    h.section_size = sh.section_size;
    h.offset_from_start = s->ptr - s->buffer;

    switch(sh.magic_number) {
        case PVD_MN_TA_FIFO_block: {
            CHECK_LEFT(sizeof(struct PVD_fifo_header), "FIFO header", return h);
            h.m.fifo = *(struct PVD_fifo_header *)s->ptr;
            s->ptr += sizeof(struct PVD_fifo_header);
            break; }
        case PVD_MN_VRAM: {
            CHECK_LEFT(sizeof(struct PVD_vram_header), "VRAM header", return h);
            h.m.vram = *(struct PVD_vram_header *)s->ptr;
            s->ptr += sizeof(struct PVD_vram_header);
            CHECK_LEFT(h.m.vram.section_size, "VRAM storage", return h);
            break; }
        case PVD_MN_game_info: {
            CHECK_LEFT(sizeof(struct PVD_game_info_header), "Game info header", return h);
            h.m.game_info = *(struct PVD_game_info_header *)s->ptr;
            s->ptr += sizeof(struct PVD_game_info_header);
            break; }
        case PVD_MN_register_update: {
            CHECK_LEFT(sizeof(struct PVD_register_update_header), "Register update header", return h);
            h.m.register_update = *(struct PVD_register_update_header *)s->ptr;
            s->ptr += sizeof(struct PVD_register_update_header);
            CHECK_LEFT((sizeof(struct PVD_register_update) * h.m.register_update.num_register_updates), "Register update storage", return h);
            break; }
        case PVD_MN_arbitrary_message: {
            CHECK_LEFT(sizeof(struct PVD_arbitrary_message_header), "Arbitrary message header", return h);
            h.m.arbitrary_message = *(struct PVD_arbitrary_message_header *)s->ptr;
            s->ptr += sizeof(struct PVD_arbitrary_message_header);
            CHECK_LEFT(h.m.arbitrary_message.size_of_message, "Storage for message", return h);
            break; }
        default:
            dbg_printf("\nUnknown section type %08x", sh.magic_number);
            h.section_size = 0;
            s->errored = 1;
            return h;
    }

    return h;
}

