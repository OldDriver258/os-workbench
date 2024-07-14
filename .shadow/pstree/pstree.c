#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#include <dirent.h>
#include <sys/types.h>
#include <malloc.h>
#include <string.h>

#define PROC_ROOT   "/proc"
#define PNODE_MAX   1000

typedef struct PChild {
    int            index;
    struct PChild *next;
} PChild;

typedef struct PNode {
    char          name[256];
    int           pid;
    PChild       *first_child;
} PNode;

PNode PNodes[PNODE_MAX];
int   PNode_num = 0;

// int add_edge(char *name, int ppid, int pid) {
    
    
// } 

int main(int argc, char *argv[]) {
    int opt, opt_index = 0;
    struct option pstree_option[] = {
        {"show-pids", no_argument, NULL, 'p'},
        {"numeric-sort", no_argument, NULL, 'n'},
        {"version", no_argument, NULL, 'V'},
        {0, 0, 0, 0}
    };
    DIR *proc_dir;
    struct dirent *procs_entry;

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

    proc_dir = opendir(PROC_ROOT);
    if (proc_dir == NULL) {
        perror("open proc dir");
        return 1;
    }

    while ((procs_entry = readdir(proc_dir))) {
        if (procs_entry->d_type == DT_DIR) {
            int pid = atoi(procs_entry->d_name);
            if (pid != 0) {
                FILE *stat_fp;
                char  stat_path[256];
                char  pname[256];
                int   ppid;

                sprintf(stat_path, PROC_ROOT"/%d/stat", pid);
                stat_fp = fopen(stat_path, "r");
                if (!stat_fp) {
                    goto release;
                }

                if (fscanf(stat_fp, "%*d (%s) %*s %d %*[^\n]", pname, &ppid) != 1) {
                    perror("fscanf ppid error");
                    goto release;
                }

                printf("find pid %d, ppid %d\n", pid, ppid);

    release:
                if (stat_fp) {
                    fclose(stat_fp);
                }
            }
        }
    }


    return 0;
}