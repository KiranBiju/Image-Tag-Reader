#include "mp3_header.h"

/* Decode a 4-byte integer */
static int decode_4byte(unsigned char *data)
{
    return ((int)(data[0] & 0x7F) << 21) |
           ((int)(data[1] & 0x7F) << 14) |
           ((int)(data[2] & 0x7F) << 7)  |
           ((int)(data[3] & 0x7F));
}

/* Decode a 4-byte big-endian integer */
static int decode_big_endian(unsigned char *data)
{
    return ((int)data[0] << 24) |
           ((int)data[1] << 16) |
           ((int)data[2] << 8)  |
           ((int)data[3]);
}

/* Encode an integer into 4-byte format */
static void encode_4byte(unsigned char *data, int value)
{
    data[0] = (value >> 21) & 0x7F;
    data[1] = (value >> 14) & 0x7F;
    data[2] = (value >> 7) & 0x7F;
    data[3] = value & 0x7F;
}

/* Encode an integer into 4-byte big-endian format */
static void encode_big_endian(unsigned char *data, int value)
{
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
}

/* Map command-line option to its corresponding ID3 frame */
static const char *get_frame_id(char *option, int version)
{
    if (strcmp(option, "-t") == 0)
        return "TIT2";

    if (strcmp(option, "-a") == 0)
        return "TPE1";

    if (strcmp(option, "-A") == 0)
        return "TALB";

    if (strcmp(option, "-y") == 0)
    {
        if (version == 3)
            return "TYER";

        return "TDRC";
    }

    if (strcmp(option, "-m") == 0)
        return "TCON";

    if (strcmp(option, "-c") == 0)
        return "COMM";

    return NULL;
}

/* Check whether the current frame is the target frame */
static int is_target_frame(char *frame_id, const char *target_id)
{
    return strcmp(frame_id, target_id) == 0;
}


static unsigned char *build_new_frame_data(struct MP3 *mp3, const char *target_id, int *new_size)
{
    size_t text_length = strlen(mp3->new_tag);

    size_t total_size;

    if (strcmp(target_id, "COMM") == 0)
    {
        total_size = 5 + text_length;
    }
    else
    {
        total_size = 1 + text_length;
    }

    if (total_size > 0x0FFFFFFF)
    {
        printf("ERROR: New tag value is too large.\n");
        return NULL;
    }

    unsigned char *data = malloc(total_size);

    if (data == NULL)
    {
        printf("ERROR: Memory allocation failed.\n");
        return NULL;
    }

    if (strcmp(target_id, "COMM") == 0)
    {
        data[0] = 0;       /* Encoding: ISO-8859-1 */
        data[1] = 'e';
        data[2] = 'n';
        data[3] = 'g';
        data[4] = '\0';   /* Empty description */

        memcpy(&data[5], mp3->new_tag, text_length);
    }
    else
    {
        data[0] = 0;       /* Encoding: ISO-8859-1 */

        memcpy(&data[1], mp3->new_tag, text_length);
    }

    *new_size = (int)total_size;

    return data;
}

/* Write a frame header */
static int write_frame_header(FILE *file, const char *frame_id, int frame_size, int version, unsigned char *original_flags)
{
    unsigned char header[FRAME_HEADER_SIZE];

    memcpy(header, frame_id, 4);

    if (version == 3)
        encode_big_endian(&header[4], frame_size);
    else
        encode_4byte(&header[4], frame_size);

    /* Preserve the original frame flags */
    header[8] = original_flags[0];
    header[9] = original_flags[1];

    return fwrite(header, 1, FRAME_HEADER_SIZE, file)
           == FRAME_HEADER_SIZE;
}

/* Copy all remaining bytes from one file to another */
static int copy_remaining_data(FILE *source, FILE *destination)
{
    unsigned char buffer[4096];

    int bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0)
    {
        if (fwrite(buffer, 1, bytes_read, destination) != bytes_read)
            return 0;
    }

    if (ferror(source))
        return 0;

    return 1;
}

/* Main edit function */
void edit(struct MP3 *mp3)
{
    unsigned char header[TAG_HEADER_SIZE];

    char temp_filename[1024];

    int version;
    int target_found = 0;
    int success = 0;

    int old_tag_size;
    int new_tag_size = 0;

    const char *target_id;

    unsigned char *new_frame_data = NULL;

    /* Open original MP3 file */
    mp3->org_mp3_fptr = fopen(mp3->mp3_filename, "rb");

    if (mp3->org_mp3_fptr == NULL)
    {
        perror("ERROR: Unable to open original MP3 file");
        return;
    }

    /* Read ID3 header */
    if (fread(header, 1, TAG_HEADER_SIZE, mp3->org_mp3_fptr)
        != TAG_HEADER_SIZE)
    {
        printf("ERROR: Unable to read ID3 header.\n");
        goto cleanup;
    }

    /* Validate ID3 identifier */
    if (memcmp(header, "ID3", 3) != 0)
    {
        printf("ERROR: No ID3 tag found.\n");
        goto cleanup;
    }

    version = header[3];

    if (version != 3 && version != 4)
    {
        printf("ERROR: Only ID3v2.3 and ID3v2.4 are supported.\n");
        goto cleanup;
    }

    /*
     * This implementation does not support extended headers,
     * unsynchronisation or ID3v2.4 footers.
     */
    if (header[5] != 0)
    {
        printf("ERROR: Unsupported ID3 tag flags.\n");
        goto cleanup;
    }

    old_tag_size = decode_4byte(&header[6]);

    if (old_tag_size > 0x0FFFFFFF)
    {
        printf("ERROR: Invalid ID3 tag size.\n");
        goto cleanup;
    }

    target_id = get_frame_id(mp3->tag_option, version);

    if (target_id == NULL)
    {
        printf("ERROR: Invalid editing option.\n");
        goto cleanup;
    }

    /* Build replacement frame data */
    int replacement_size = 0;

    new_frame_data = build_new_frame_data(mp3, target_id, &replacement_size);

    if (new_frame_data == NULL)
        goto cleanup;

    /* Create temporary filename */
    if (snprintf(temp_filename,
                 sizeof(temp_filename),
                 "%s.tmp",
                 mp3->mp3_filename) >= (int)sizeof(temp_filename))
    {
        printf("ERROR: Temporary filename is too long.\n");
        goto cleanup;
    }

    /* Open temporary file */
    mp3->dup_mp3_fptr = fopen(temp_filename, "wb");

    if (mp3->dup_mp3_fptr == NULL)
    {
        perror("ERROR: Unable to create temporary file");
        goto cleanup;
    }

    /*
     * Write placeholder ID3 header.
     * We will update its tag size after writing all frames.
     */
    if (fwrite(header, 1, TAG_HEADER_SIZE, mp3->dup_mp3_fptr)
        != TAG_HEADER_SIZE)
    {
        printf("ERROR: Unable to write temporary ID3 header.\n");
        goto cleanup;
    }

    int remaining = old_tag_size;

    while (remaining >= FRAME_HEADER_SIZE)
    {
        unsigned char frame_header[FRAME_HEADER_SIZE];

        if (fread(frame_header, 1, FRAME_HEADER_SIZE,
                  mp3->org_mp3_fptr) != FRAME_HEADER_SIZE)
        {
            printf("ERROR: Unable to read frame header.\n");
            goto cleanup;
        }

        remaining -= FRAME_HEADER_SIZE;

        /*
         * Padding starts with zero bytes.
         * We do not copy the old padding because the new tag
         * size will be calculated from the frames we write.
         */
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
            goto cleanup;
        }

        /*
         * Read the existing frame data.
         * This also ensures that the source pointer advances
         * past the old frame before processing the next one.
         */
        unsigned char *frame_data = malloc(frame_size);

        if (frame_data == NULL && frame_size > 0)
        {
            printf("ERROR: Memory allocation failed.\n");
            goto cleanup;
        }

        if (frame_size > 0 &&
            fread(frame_data, 1, frame_size, mp3->org_mp3_fptr)
            != frame_size)
        {
            printf("ERROR: Unable to read frame data.\n");
            free(frame_data);
            goto cleanup;
        }

        remaining -= frame_size;

        if (is_target_frame(frame_id, target_id))
        {
            if (target_found)
            {
                printf("ERROR: Duplicate target frames found.\n");
                free(frame_data);
                goto cleanup;
            }

            /*
             * Do not edit a target frame with flags set.
             * Such flags can indicate special frame formats.
             */
            if (frame_header[8] != 0 || frame_header[9] != 0)
            {
                printf("ERROR: Target frame has unsupported flags.\n");
                free(frame_data);
                goto cleanup;
            }

            /* Write replacement frame header */
            if (!write_frame_header(mp3->dup_mp3_fptr,
                                    target_id,
                                    replacement_size,
                                    version,
                                    &frame_header[8]))
            {
                printf("ERROR: Unable to write replacement frame header.\n");
                free(frame_data);
                goto cleanup;
            }

            /* Write replacement frame data */
            if (fwrite(new_frame_data, 1, replacement_size,
                       mp3->dup_mp3_fptr) != replacement_size)
            {
                printf("ERROR: Unable to write replacement frame data.\n");
                free(frame_data);
                goto cleanup;
            }

            target_found = 1;
        }
        else
        {
            /*
             * Preserve all other frames exactly as they were.
             */
            if (fwrite(frame_header, 1, FRAME_HEADER_SIZE,
                       mp3->dup_mp3_fptr) != FRAME_HEADER_SIZE)
            {
                printf("ERROR: Unable to copy frame header.\n");
                free(frame_data);
                goto cleanup;
            }

            if (frame_size > 0 &&
                fwrite(frame_data, 1, frame_size,
                       mp3->dup_mp3_fptr) != frame_size)
            {
                printf("ERROR: Unable to copy frame data.\n");
                free(frame_data);
                goto cleanup;
            }
        }

        free(frame_data);
    }

    if (!target_found)
    {
        printf("ERROR: Requested tag frame '%s' was not found.\n",
               target_id);

        printf("This implementation edits existing frames only.\n");

        goto cleanup;
    }

    /*
     * Calculate new tag size.
     *
     * The current output position is immediately after the
     * frames we copied or replaced. The 10-byte ID3 header
     * is not included in the tag size.
     */
    long current_position = ftell(mp3->dup_mp3_fptr);

    if (current_position < TAG_HEADER_SIZE)
    {
        printf("ERROR: Unable to calculate new tag size.\n");
        goto cleanup;
    }

    new_tag_size = (int)(current_position - TAG_HEADER_SIZE);

    if (new_tag_size > 0x0FFFFFFF)
    {
        printf("ERROR: New ID3 tag is too large.\n");
        goto cleanup;
    }

    /*
     * Move source pointer to the end of the original ID3 tag.
     * This skips any remaining padding bytes.
     */
    if (fseek(mp3->org_mp3_fptr,
              TAG_HEADER_SIZE + (long)old_tag_size,
              SEEK_SET) != 0)
    {
        printf("ERROR: Unable to locate audio data.\n");
        goto cleanup;
    }

    /* Copy original audio data */
    if (!copy_remaining_data(mp3->org_mp3_fptr,
                             mp3->dup_mp3_fptr))
    {
        printf("ERROR: Unable to copy audio data.\n");
        goto cleanup;
    }

    /*
     * Update the ID3 header with the new tag size.
     */
    encode_4byte(&header[6], new_tag_size);

    if (fseek(mp3->dup_mp3_fptr, 0, SEEK_SET) != 0)
    {
        printf("ERROR: Unable to update ID3 header.\n");
        goto cleanup;
    }

    if (fwrite(header, 1, TAG_HEADER_SIZE,
               mp3->dup_mp3_fptr) != TAG_HEADER_SIZE)
    {
        printf("ERROR: Unable to write updated ID3 header.\n");
        goto cleanup;
    }

    /* Ensure all output data is written successfully */
    if (fflush(mp3->dup_mp3_fptr) != 0)
    {
        printf("ERROR: Unable to flush temporary file.\n");
        goto cleanup;
    }

    success = 1;

cleanup:

    if (mp3->org_mp3_fptr != NULL)
    {
        fclose(mp3->org_mp3_fptr);
        mp3->org_mp3_fptr = NULL;
    }

    if (mp3->dup_mp3_fptr != NULL)
    {
        if (fclose(mp3->dup_mp3_fptr) != 0)
            success = 0;

        mp3->dup_mp3_fptr = NULL;
    }

    free(new_frame_data);

    if (success)
    {
        /*
         * Replace original file with the temporary file.
         * On POSIX systems, rename replaces an existing
         * destination file atomically when both are on the
         * same filesystem.
         */
        if (rename(temp_filename, mp3->mp3_filename) != 0)
        {
            perror("ERROR: Unable to replace original MP3 file");
            remove(temp_filename);
            return;
        }

        printf("\nTag updated successfully!\n");
        printf("Updated frame : %s\n", target_id);
        printf("New value     : %s\n", mp3->new_tag);
    }
    else
    {
        remove(temp_filename);
        printf("\nTag editing failed. Original file was not replaced.\n");
    }
}