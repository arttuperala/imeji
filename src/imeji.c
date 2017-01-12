#include "imeji.h"

// Compare the beginning identifier to the ID3 header.
// Returns 1 if ID3 identifier is found and 0 if the identifier is not found.
int check_id3_identifier(const char* filepath) {
    static char ID3_identifier[3] = {0x49, 0x44, 0x33};
    FILE *file;
    char file_identifier[3];
    char valid_identifier = 1;

    file = fopen(filepath, "r");
    if (file == NULL) {
        valid_identifier = 0;
    } else {
        fseek(file, 0, SEEK_SET);
        fread(&file_identifier, 1, 3, file);
        for (int i = 0; i < 3; ++i) {
            if (file_identifier[i] != ID3_identifier[i]) {
                valid_identifier = 0;
            }
        }
    }

    fclose(file);
    return valid_identifier;
}

// Returns a non-synchsafe ID3v2 integer as a long.
long read_size_integer(char bytes[]) {
    long total = 0;
    for (int i = 0; i < 4; ++i) {
        total += (bytes[i] & 0xFF) << (3-i)*8;
    }
    return total;
}

// Returns a synchsafe ID3v2 integer as a long.
long read_synchsafe_size_integer(char bytes[]) {
    long total = 0;
    for (int i = 0; i < 4; ++i) {
        total += (bytes[i] & 0x7F) << (3-i)*7;
    }
    return total;
}

// Returns a ID3Header struct with relevant information needed for scraping APIC frames.
void read_id3_header(FILE* file, struct ID3Header* header) {
    fread(&header->major_version, 1, 1, file);
    fread(&header->minor_version, 1, 1, file);
    fseek(file, 1, SEEK_CUR);

    char size_bytes[4];
    fread(&size_bytes, 4, 1, file);
    long size = read_synchsafe_size_integer(size_bytes);
    header->size = size;
}

// Searches through the ID3 tag for the first APIC frame.
// Returns 1 when an APIC frame is found and 0 when no APIC frame is found.
// Size and position of the APIC frame is stored in the out_frame.
int find_APIC(FILE* file, struct ID3Header* header, struct ID3Frame* out_frame) {
    static char frame_null[4] = {0x00, 0x00, 0x00, 0x00};
    static char frame_apic[4] = {0x41, 0x50, 0x49, 0x43};

    fpos_t file_pos;
    fgetpos(file, &file_pos);
    while (file_pos < header->size) {
        char frame_identifier[4];
        char size_bytes[4];
        char flags[2];
        struct ID3Frame frame;

        fread(&frame_identifier, 4, 1, file);
        fread(&size_bytes, 4, 1, file);
        fread(&flags, 2, 1, file);

        if (header->major_version >= 4) {
            frame.size = read_synchsafe_size_integer(size_bytes);
        } else {
            frame.size = read_size_integer(size_bytes);
        }
        fgetpos(file, &frame.position);

        if (memcmp(frame_identifier, frame_apic, 4) == 0 ) {
            memcpy(out_frame, &frame, sizeof(frame));
            return 1;
        } else if (memcmp(frame_identifier, frame_null, 4) == 0) {
            return 0;
        } else {
            fseek(file, frame.size, SEEK_CUR);
        }

        fgetpos(file, &file_pos);
    }
    return 0;
}

void read_APIC(FILE* file, struct ID3Frame APIC_frame, struct PictureData* picture_data) {
    int text_encoding = 0;
    int picture_type = 0;
    int text_encoding_incrementation = 1;

    fseek(file, APIC_frame.position, SEEK_SET);
    fread(&text_encoding, 1, 1, file);
    if (text_encoding == 1 || text_encoding == 2) {
        text_encoding_incrementation = 2;
    }
    while (1) {
        char mime_type_char;
        fread(&mime_type_char, 1, 1, file);
        if (mime_type_char == 0x00) {
            break;
        }
    }
    fread(&picture_type, 1, 1, file);
    while (1) {
        char text_string_bytes[text_encoding_incrementation];
        int null_bytes = 0;
        fread(&text_string_bytes, text_encoding_incrementation, 1, file);
        for (int i = 0; i < text_encoding_incrementation; ++i) {
            if (text_string_bytes[i] == 0x00) {
                null_bytes += 1;
            }
        }
        if (null_bytes == text_encoding_incrementation) {
            break;
        }
    }

    fpos_t position;
    fgetpos(file, &position);
    picture_data->size = APIC_frame.size - (position - APIC_frame.position);
    picture_data->data = (char*)malloc(sizeof(char)*picture_data->size);
    fread(picture_data->data, picture_data->size, 1, file);
}

struct PictureData get_id3_picture_data(const char* filepath) {
    FILE *file;
    struct PictureData apic;
    struct ID3Frame frame;
    struct ID3Header header;

    apic.size = 0;

    file = fopen(filepath, "r");
    if (file == NULL) {
        return apic;
    }
    fseek(file, 3, SEEK_SET);

    read_id3_header(file, &header);
    if (header.major_version < 3) {
        return apic;
    }

    if (find_APIC(file, &header, &frame)) {
        read_APIC(file, frame, &apic);
    }
    fclose(file);
    return apic;
}

void free_picture_data(struct PictureData picture_data) {
    free(picture_data.data);
}
