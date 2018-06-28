#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

void print_help()
{
    printf("command:\n");
    printf("\t-d: package full path.\n");
    printf("\t-o: unpack output path.\n");
    printf("\t-h: Help.\n");
}
int main(int argc, char *argv[])
{
    int c;
    int isExit = 0;

    while ((c = getopt(argc, argv, "d:o:h")) != -1)
    {
        switch (c)
        {
        case 'd':
            //pPackageFullPath = optarg;
            break;
        case 'o':
            //pFrmOutputPath = optarg;
            break;
        case 'h':
            print_help();
            isExit = 1;
            break;
        default:
            print_help();
            isExit = 1;
            break;
        }
    }
    if (isExit) {
        exit(0);
    }
    return 0;
}