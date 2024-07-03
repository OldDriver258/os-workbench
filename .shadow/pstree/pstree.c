#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
    int opt, opt_index = 0;
    struct option pstree_option[] = {
        {"show-pids", no_argument, NULL, 'p'},
        {"numeric-sort", no_argument, NULL, 'n'},
        {"version", no_argument, NULL, 'V'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "pnV", pstree_option, &opt_index)) != -1) {
        switch (opt)
        {
        case 'p':
            printf("find p flag\n");
            break;
        
        case 'n':
            printf("find n flag\n");
            break;

        case 'V':
            fprintf(stderr, "pstree 0.1\n");
            return 0;

        default:
            fprintf(stderr, "Usage: pstree [-p --show-pids] [-n --numeric-sort]\n");
            fprintf(stderr, "       pstree -V --version\n");
            return 1;
        }
    }

    return 0;
}