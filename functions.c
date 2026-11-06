#include "mp3_header.h"

static void print_help(void)
{
    printf("\n========== MP3 TAG READER ==========\n");

    printf("\nVIEW MODE:\n");
    printf("./a.out -v <mp3_filename>\n");

    printf("\nEDIT MODE:\n");
    printf("./a.out -e <option> <new_value> <mp3_filename>\n");

    printf("\nOPTIONS:\n");
    printf("-t  Edit Title\n");
    printf("-a  Edit Artist\n");
    printf("-A  Edit Album\n");
    printf("-y  Edit Year\n");
    printf("-m  Edit Genre\n");
    printf("-c  Edit Comment\n");

    printf("\nHELP:\n");
    printf("./a.out --help\n");

    printf("\n====================================\n");
}

static int validate_edit_option(char *option)
{
    if (strcmp(option, "-t") == 0 ||
        strcmp(option, "-a") == 0 ||
        strcmp(option, "-A") == 0 ||
        strcmp(option, "-y") == 0 ||
        strcmp(option, "-m") == 0 ||
        strcmp(option, "-c") == 0)
    {
        return 1;
    }

    return 0;
}

int validate_cla(int argc, char *argv[], struct MP3 *mp3)
{
    if (argc < 2)
    {
        print_help();
        return FAILURE;
    }

    /* HELP MODE */
    if (strcmp(argv[1], "--help") == 0)
    {
        if (argc != 2)
        {
            printf("ERROR: Invalid arguments for help.\n");
            print_help();
            return FAILURE;
        }

        print_help();
        return FAILURE;
    }

    /* VIEW MODE */
    if (strcmp(argv[1], "-v") == 0)
    {
        if (argc != 3)
        {
            printf("ERROR: Invalid arguments for view mode.\n");
            print_help();
            return FAILURE;
        }

        mp3->mp3_filename = argv[2];

        return VIEW;
    }

    /* EDIT MODE */
    if (strcmp(argv[1], "-e") == 0)
    {
        if (argc != 5)
        {
            printf("ERROR: Invalid arguments for edit mode.\n");
            print_help();
            return FAILURE;
        }

        if (!validate_edit_option(argv[2]))
        {
            printf("ERROR: Invalid editing option '%s'.\n", argv[2]);
            print_help();
            return FAILURE;
        }

        if (argv[3][0] == '\0')
        {
            printf("ERROR: New tag value cannot be empty.\n");
            return FAILURE;
        }

        mp3->tag_option = argv[2];
        mp3->new_tag = argv[3];
        mp3->mp3_filename = argv[4];

        return EDIT;
    }

    printf("ERROR: Invalid operation '%s'.\n", argv[1]);
    print_help();

    return FAILURE;
}