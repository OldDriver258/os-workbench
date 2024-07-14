#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TreeNode {
    int value;
    struct TreeNode **children;
    int child_count;
} TreeNode;

TreeNode* create_node(int value) {
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    node->value = value;
    node->children = NULL;
    node->child_count = 0;
    return node;
}

void add_child(TreeNode *parent, TreeNode *child) {
    parent->child_count++;
    parent->children = (TreeNode **)realloc(parent->children, parent->child_count * sizeof(TreeNode *));
    parent->children[parent->child_count - 1] = child;
}

TreeNode* build_tree(int edges[][2], int edge_count) {
    TreeNode **nodes = (TreeNode **)malloc(100 * sizeof(TreeNode *));
    for (int i = 0; i < 100; i++) {
        nodes[i] = NULL;
    }

    for (int i = 0; i < edge_count; i++) {
        int parent_value = edges[i][0];
        int child_value = edges[i][1];

        if (nodes[parent_value] == NULL) {
            nodes[parent_value] = create_node(parent_value);
        }
        if (nodes[child_value] == NULL) {
            nodes[child_value] = create_node(child_value);
        }
        add_child(nodes[parent_value], nodes[child_value]);
    }

    TreeNode *root = nodes[edges[0][0]];
    free(nodes);
    return root;
}

void generate_bracket_sequence(TreeNode *node, char *output) {
    char buffer[20];
    sprintf(buffer, "%d", node->value);
    strcat(output, buffer);

    if (node->child_count > 0) {
        strcat(output, "(");
        for (int i = 0; i < node->child_count; i++) {
            generate_bracket_sequence(node->children[i], output);
            if (i < node->child_count - 1) {
                strcat(output, ",");
            }
        }
        strcat(output, ")");
    }
}

void generate_tree_representation(TreeNode *node, int depth, int is_last, char *prefix) {
    char buffer[100];
    if (depth == 0) {
        sprintf(buffer, "%d\n", node->value);
    } else {
        char connector[3] = "+-";
        if (!is_last) {
            sprintf(buffer, "%s|%s%d\n", prefix, connector, node->value);
        } else {
            sprintf(buffer, "%s+-%d\n", prefix, node->value);
        }
    }
    printf("%s", buffer);

    char new_prefix[100];
    sprintf(new_prefix, "%s  ", prefix);
    for (int i = 0; i < node->child_count; i++) {
        generate_tree_representation(node->children[i], depth + 1, (i == node->child_count - 1), new_prefix);
    }
}

int main() {
    int edges[][2] = {
        {1, 2},
        {2, 3},
        {2, 4},
        {3, 5},
        {4, 6},
        {3, 7},
        {1, 8}
    };
    int edge_count = sizeof(edges) / sizeof(edges[0]);

    TreeNode *root = build_tree(edges, edge_count);

    char bracket_sequence[1000] = "";
    generate_bracket_sequence(root, bracket_sequence);
    printf("括号序列表示:\n%s\n", bracket_sequence);

    printf("\n树的图形化表示:\n");
    generate_tree_representation(root, 0, 1, "");

    return 0;
}
