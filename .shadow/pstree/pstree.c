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

int pnode_list_add_end (PNode* pnode, int index) {
    PChild **child = &pnode->first_child;

    while (*child) {
        child = &(*child)->next;
    }

    *child = (PChild*)malloc(sizeof(PChild));
    (*child)->index = index;
    (*child)->next  = NULL;

    return 0;
}

int add_edge (char *name, int ppid, int pid) {
    // int index;

    // for (index = 0; index < PNode_num; index++) {
    //     if (PNodes[index].pid == pid) {
    //         break;
    //     }
    // }

    // if (index < PNode_num) {
    //     // 已经存在的节点
    // } else {
    //     // 新的节点
    //     strcpy(PNodes[index].name, name);
    //     PNodes[index].pid = pid;
    //     PNode_num++;
    // }

    strcpy(PNodes[PNode_num].name, name);
    PNodes[PNode_num].pid = pid;

    for (int i = 0; i < PNode_num; i++) {
        if (PNodes[i].pid == ppid) {
            pnode_list_add_end(&PNodes[i], PNode_num);
        }
    }

    PNode_num++;

    //test
    // for (int i = 0; i < PNode_num; i++)
    // {
    //     printf ("i:%d, name:%s, pid:%d\n", i, PNodes[i].name, PNodes[i].pid);
    // }
    

    // if (i < PNode_num) {
    //     // 已经存在的节点
        
    // } else {
    //     // 新的节点
    // }

    return 0;
}

int dfs_print (int index) {
    PNode *pnode = &PNodes[index];
    PChild *pchild = pnode->first_child;

    printf("%d", pnode->pid);

    if (pchild) {
        printf("(");
        while (pchild != NULL) {
            dfs_print(pchild->index);
            pchild = pchild->next;
            if (pchild) {
                 printf(",");
            }
        }
        printf(")");
    }

    return 0;
}

int main (int argc, char *argv[]) {
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

                if (fscanf(stat_fp, "%*d %s %*s %d %*[^\n]", pname, &ppid) != 2) {
                    perror("fscanf ppid error");
                    goto release;
                }

                // 添加有向边
                printf("find %s pid %d, ppid %d\n", pname, pid, ppid);
                // add_edge(pname, ppid, pid);

    release:
                if (stat_fp) {
                    fclose(stat_fp);
                }
            }
        }
    }

    add_edge("test0", 0, 1);
    add_edge("test1", 1, 2);
    add_edge("test2", 2, 3);
    add_edge("test3", 2, 4);
    add_edge("test4", 3, 5);
    add_edge("test5", 4, 6);
    add_edge("test6", 3, 7);
    add_edge("test6", 1, 8);
    // dfs 打印输出
    dfs_print(0);

    return 0;
}