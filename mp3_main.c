#include "mp3_header.h"

int main(int argc, char *argv[])
{
    struct MP3 mp3 = {0};

    int ret = validate_cla(argc, argv, &mp3);

    switch(ret)
    {
        case VIEW:
            view(&mp3);
            break;

        case EDIT:
            edit(&mp3);
            break;

        case FAILURE:
            return EXIT_FAILURE;

        default:
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}