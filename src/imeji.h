#ifndef imeji_h
#define imeji_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ID3Header {
    int8_t major_version;
    int8_t minor_version;
    long size;
};

struct ID3Frame {
    fpos_t position;
    long size;
};

struct PictureData {
    long size;
    char *data;
};

int check_id3_identifier(const char* filepath);

long read_size_integer(char bytes[]);

long read_synchsafe_size_integer(char bytes[]);

void read_id3_header(FILE* file, struct ID3Header* header);

int find_APIC(FILE* file, struct ID3Header* header, struct ID3Frame* out_frame);

void read_APIC(FILE* file, struct ID3Frame APIC_frame, struct PictureData* picture_data);

extern struct PictureData get_id3_picture_data(const char* filepath);

extern void free_picture_data(struct PictureData picture_data);

#endif /* imeji_h */
