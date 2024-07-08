//
// Created by . on 7/8/24.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

    if (S_BYTES_LEFT < sizeof(s->file_header)) {
        dbg_printf("\nExpected at least %li bytes free, have %ld!", sizeof(struct PVD_file_header), S_BYTES_LEFT);
        free(s);
        return NULL;
    }

    s->file_header = ((struct PVD_file_header *)s->ptr);

    if (s->file_header->magic_number != PVD_MN_file) {
        dbg_printf("\nFound bad magic number! %08x", s->file_header->magic_number);
        free(s);
        return NULL;
    }

    if (s->file_header->total_size < inbuffer_len) {
        dbg_printf("\nHeader says file is %d bytes, but input buffer is only %d!", s->file_header->total_size, inbuffer_len);
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
    if (outbuffer_len < sizeof(struct PVD_file_header)) {
        dbg_printf("\nNeed buffer of at least size %d!", (int)sizeof(struct PVD_file_header));
        return NULL;
    }

    struct PVD_reader_writer *s = malloc(sizeof(struct PVD_reader_writer));

    s->errored = 0;
    s->buffer = (uint8_t *)outbuffer;
    s->buffer_size = outbuffer_len;
    s->ptr = s->buffer;

    s->file_header = (struct PVD_file_header *)s->ptr;
    s->ptr += sizeof(struct PVD_file_header);
    s->file_header->magic_number = PVD_MN_file;
    s->file_header->total_size = 16;
    s->file_header->offset_to_first_block = 16;
    s->file_header->version = 0;

    return s;
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

// Returns: # of bytes read. 0 if none read. negative if errored.
int PVD_read_FIFO_VRAM_arbitrary_message(struct PVD_reader_writer *s, struct PVD_header *h, char *outbuffer, uint32_t outbuffer_len)
{
    int64_t to_read = -1;
    switch(h->m.section.magic_number) {
        case PVD_MN_register_update: {
            to_read = (int64_t)h->m.register_update.num_register_updates * (int64_t)sizeof(struct PVD_register_update);
            CHECK_LEFT(to_read, "register update storage", return -1);
            break; }
        case PVD_MN_file: {
            return 0; }
        case PVD_MN_arbitrary_message: {
            to_read = (int64_t)h->m.arbitrary_message.size_of_message;
            CHECK_LEFT(to_read, "arbitrary message storage", return -1);
            break; }
        case PVD_MN_game_info: {
            return 0; }
        case PVD_MN_VRAM: {
            to_read = (int64_t)h->m.vram.length;
            CHECK_LEFT(to_read, "VRAM storage", return -1);
            break; }
        case PVD_MN_TA_FIFO_block: {
            to_read = (int64_t)h->m.fifo.length;
            CHECK_LEFT(to_read, "TA FIFO storage", return -1);
            break; }
        default: {
            dbg_printf("\nUnknown magic number %08x", h->m.section.magic_number);
            s->errored = 1;
            return -1;
        }
    }
    if (to_read > outbuffer_len) {
        dbg_printf("\nNeeded %lld bytes for bufffer, only have %d!", to_read, outbuffer_len);
        s->errored = 1;
        return -1;
    }
    memcpy(outbuffer, s->ptr, to_read);
    s->ptr += to_read;
    return (int)to_read;
}

struct PVD_register_update PVD_read_single_register_update(struct PVD_reader_writer *s, struct PVD_header *h)
{
    struct PVD_register_update r = (struct PVD_register_update){};
    CHECK_LEFT(sizeof(struct PVD_register_update), "register update", return r);
    uint8_t* old_ptr = s->ptr;
    s->ptr += sizeof(struct PVD_register_update);
    return *(struct PVD_register_update *)old_ptr;
}


void PVD_write_game_info(struct PVD_reader_writer *s, const char*name, const char*filename, uint64_t frame_number)
{
    CHECK_LEFT(128, "game info", return);
    uint32_t name_len = 0, filename_len = 0;
    if (name)
        name_len = strnlen(name, 49);
    if (filename)
        filename_len = strnlen(filename, 49);
    struct PVD_game_info_header *h = (struct PVD_game_info_header *)s->ptr;
    s->ptr += sizeof(struct PVD_game_info_header);
    h->section_size = 128;
    h->magic_number = PVD_MN_game_info;
    h->version = 0;
    h->name_length = name_len;
    h->filename_length = filename_len;
    h->frame = (int64_t)frame_number;
    memset(h->name, 0, 50);
    memset(h->filename, 0, 50);

    if (name) memcpy(h->name, name, name_len);
    if (filename) memcpy(h->filename, filename, filename_len);

    s->file_header->total_size += h->section_size;
}

void PVD_write_arbitrary_message(struct PVD_reader_writer *s, char *inbuf, uint32_t inbuf_len)
{
    CHECK_LEFT(16 + inbuf_len, "game info", return);
    struct PVD_arbitrary_message_header *h = (struct PVD_arbitrary_message_header *)s->ptr;
    s->ptr += sizeof(struct PVD_arbitrary_message_header);

    h->section_size = 16 + inbuf_len;
    h->size_of_message = inbuf_len;
    h->magic_number = PVD_MN_arbitrary_message;
    h->version = 0;

    memcpy(s->ptr, inbuf, inbuf_len);
    s->ptr += inbuf_len;

    s->file_header->total_size += h->section_size;
}

void PVD_write_register_update(struct PVD_reader_writer *s, uint32_t num_updates, struct PVD_register_update *updates)
{
    CHECK_LEFT(16 + (sizeof(struct PVD_register_update) * num_updates), "register updates", return);
    struct PVD_register_update_header *h = (struct PVD_register_update_header *)s->ptr;
    s->ptr += sizeof(struct PVD_register_update_header);
    h->section_size = 16 + (num_updates * sizeof(struct PVD_register_update));
    h->version = 0;
    h->magic_number = PVD_MN_register_update;
    h->num_register_updates = num_updates;
    memcpy(s->ptr, updates, (num_updates * sizeof(struct PVD_register_update)));
    s->ptr += (num_updates * sizeof(struct PVD_register_update));

    s->file_header->total_size += h->section_size;
}

void PVD_write_VRAM(struct PVD_reader_writer *s, uint32_t offset, uint32_t len, char *inbuffer)
{
    CHECK_LEFT(20 + len, "VRAM block", return);
    struct PVD_vram_header *h = (struct PVD_vram_header *)s->ptr;
    s->ptr += sizeof(struct PVD_vram_header);

    h->section_size = 20 + len;
    h->start_offset = offset;
    h->length = len;
    h->version = 0;
    h->magic_number = PVD_MN_VRAM;

    memcpy(s->ptr, inbuffer, len);
    s->ptr += len;

    s->file_header->total_size += h->section_size;
}

void PVD_write_FIFO(struct PVD_reader_writer *s, uint32_t offset, char *inbuffer, uint32_t inbuffer_len)
{
    CHECK_LEFT(20 + inbuffer_len, "FIFO block", return);
    struct PVD_fifo_header *h = (struct PVD_fifo_header *)s->ptr;
    s->ptr += sizeof(struct PVD_fifo_header);

    h->section_size = 20 + inbuffer_len;
    h->version = 0;
    h->length = inbuffer_len;
    h->magic_number = PVD_MN_TA_FIFO_block;
    h->flags = 0;

    memcpy(s->ptr, inbuffer, inbuffer_len);

    s->file_header->total_size += h->section_size;
}

void PVD_delete_reader_writer(struct PVD_reader_writer *s)
{
    if (s != NULL) free(s);
}