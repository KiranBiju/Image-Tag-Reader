#include "mp3_header.h"

/* Convert a 4-byte integer into a normal integer */
static int decode_4byte(unsigned char *data)
{
    return ((int)(data[0] & 0x7F) << 21) |
           ((int)(data[1] & 0x7F) << 14) |
           ((int)(data[2] & 0x7F) << 7)  |
           ((int)(data[3] & 0x7F));
}

/* Convert a 4-byte big-endian integer into a normal integer */
static int decode_big_endian(unsigned char *data)
{
    return ((int)data[0] << 24) |
           ((int)data[1] << 16) |
           ((int)data[2] << 8)  |
           ((int)data[3]);
}

/* Convert a frame ID into a readable tag name */
static const char *tag_to_name(char *frame_id, int version)
{
    if (strcmp(frame_id, "TIT2") == 0)
        return "Title";

    if (strcmp(frame_id, "TPE1") == 0)
        return "Artist";

    if (strcmp(frame_id, "TALB") == 0)
        return "Album";

    if (strcmp(frame_id, "TYER") == 0 ||
        strcmp(frame_id, "TDRC") == 0)
        return "Year";

    if (strcmp(frame_id, "TCON") == 0)
        return "Genre";

    if (strcmp(frame_id, "COMM") == 0)
        return "Comment";

    return NULL;
}

//Print text data from a text frame
static void print_text_frame(char *frame_id, unsigned char *data, int size)
{
    if (size < 1)
        return;

    unsigned char encoding = data[0];

    printf("%-10s : ", tag_to_name(frame_id, 3));

    if (encoding == 0 || encoding == 3)
    {
        for (int i = 1; i < size; i++)
        {
            if (data[i] == '\0')
                break;

            putchar(data[i]);
        }
    }
    else
    {
        printf("[Unsupported text encoding]");
    }

    printf("\n");
}

/* Print the contents of a COMM frame */
static void print_comment_frame(unsigned char *data, int size)
{
    if (size < 4)
        return;

    unsigned char encoding = data[0];

    printf("%-10s : ", "Comment");

    if (encoding != 0 && encoding != 3)
    {
        printf("[Unsupported text encoding]\n");
        return;
    }

    int i = 4;

    while (i < size && data[i] != '\0')
    {
        i++;
    }

    if (i < size)
        i++;

    while (i < size && data[i] != '\0')
    {
        putchar(data[i]);
        i++;
    }

    printf("\n");
}

/* Read and print one frame */
static void print_frame(char *frame_id, unsigned char *data, int size)
{
    const char *tag_name = tag_to_name(frame_id, 3);

    if (tag_name == NULL)
        return;

    if (strcmp(frame_id, "COMM") == 0)
    {
        print_comment_frame(data, size);
    }
    else
    {
        print_text_frame(frame_id, data, size);
    }
}

/* Main view function */
void view(struct MP3 *mp3)
{
    unsigned char header[TAG_HEADER_SIZE];

    mp3->org_mp3_fptr = fopen(mp3->mp3_filename, "rb");

    if (mp3->org_mp3_fptr == NULL)
    {
        perror("ERROR: Unable to open MP3 file");
        return;
    }

    /* Read the 10-byte ID3 header */
    if (fread(header, 1, TAG_HEADER_SIZE, mp3->org_mp3_fptr)
        != TAG_HEADER_SIZE)
    {
        printf("ERROR: Unable to read ID3 header.\n");
        fclose(mp3->org_mp3_fptr);
        return;
    }

    /* Check ID3 identifier */
    if (memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: No ID3 tag found.\n");
        fclose(mp3->org_mp3_fptr);
        return;
    }

    int version = header[3];

    if (version != 3 && version != 4)
    {
        printf("ERROR: Only ID3v2.3 and ID3v2.4 are supported.\n");
        fclose(mp3->org_mp3_fptr);
        return;
    }

    /*
     * This implementation does not process extended headers,
     * unsynchronisation or ID3v2.4 footers.
     */
    if (header[5] != 0)
    {
        printf("ERROR: Unsupported ID3 tag flags.\n");
        fclose(mp3->org_mp3_fptr);
        return;
    }

    int tag_size = decode_4byte(&header[6]);

    printf("\n========== MP3 TAG DETAILS ==========\n");
    printf("File    : %s\n", mp3->mp3_filename);
    printf("Version : ID3v2.%d\n\n", version);

    int remaining = tag_size;

    while (remaining >= FRAME_HEADER_SIZE)
    {
        unsigned char frame_header[FRAME_HEADER_SIZE];

        if (fread(frame_header, 1, FRAME_HEADER_SIZE,
                  mp3->org_mp3_fptr) != FRAME_HEADER_SIZE)
        {
            printf("ERROR: Unable to read frame header.\n");
            break;
        }

        remaining -= FRAME_HEADER_SIZE;

        /* Padding begins when the frame ID starts with zero */
        if (frame_header[0] == 0)
            break;

        char frame_id[5];

        memcpy(frame_id, frame_header, 4);
        frame_id[4] = '\0';

        int frame_size;

        if (version == 3)
            frame_size = decode_big_endian(&frame_header[4]);
        else
            frame_size = decode_4byte(&frame_header[4]);

        if (frame_size > remaining)
        {
            printf("ERROR: Invalid frame size in %s.\n", frame_id);
            break;
        }

        unsigned char *frame_data = malloc(frame_size);

        if (frame_data == NULL && frame_size > 0)
        {
            printf("ERROR: Memory allocation failed.\n");
            break;
        }

        if (frame_size > 0 &&
            fread(frame_data, 1, frame_size, mp3->org_mp3_fptr)
            != frame_size)
        {
            printf("ERROR: Unable to read frame data.\n");
            free(frame_data);
            break;
        }

        remaining -= frame_size;

        /*
         * The frame flags are not interpreted here.
         * Skip frames with format flags set.
         */
        if (frame_header[9] == 0)
        {
            print_frame(frame_id, frame_data, frame_size);
        }

        free(frame_data);
    }

    printf("\n=====================================\n");

    fclose(mp3->org_mp3_fptr);
}