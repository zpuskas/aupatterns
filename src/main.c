/*
 * Andorid unlock pattern calculator.
 * Copyright (C) 2011  Zoltan Puskas
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Maintainer: Zoltan Puskas <zoltan@sinustrom.info>
 * Created on: 2011.10.10.
 *
 * The programs purpose is to calculate all possible valid Android unlock 
 * patterns. This is used for informational purposes (my intrest). Also this
 * program can be used to generate a random pattern to use on the phone 
 * (something like `pwgen`).
 *
 * On the Android lock screen there are 9 dots arranged in a 3x3 matrix. A 
 * pattern of arbirtary length must be drawn on the screen in order to unlock
 * the phone.
 * The following restrictions apply to the pattern drawing:
 *  - any point can be used only once
 *  - pattern must contain minimum 4 points
 *  - pattern can contain maximum 9 points
 *  - cannot jump over neighboring points (e.g. moving finger from point 1 to 3
 *    will automatically connect point 2 to form pattern 1-2-3, unless 2 is
 *    alreay used in which case ..-1-3-.. is possible).
 *
 */

/*
 * TODO:
 * - print patterns to file
 * - print random patterns to screen (for generation)
 * - print valid patterns if dots are given (try to guess the probale one?)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Number of points in the pattern which 
 * (it is also the maximum depth for the tree) */
#define MAX_POINTS 9

/* Matrix describing which transition is blocked by which node */
const int block_matrix[10][10] = {
   /*0, 1, 2, 3, 4, 5, 6, 7, 8, 9 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* 0 */
    {0, 0, 0, 2, 0, 0, 0, 4, 0, 5}, /* 1 */
    {0, 0, 0, 0, 0, 0, 0, 0, 5, 0}, /* 2 */
    {0, 2, 0, 0, 0, 0, 0, 5, 0, 6}, /* 3 */
    {0, 0, 0, 0, 0, 0, 5, 0, 0, 0}, /* 4 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* 5 */
    {0, 0, 0, 0, 5, 0, 0, 0, 0, 0}, /* 6 */
    {0, 4, 0, 5, 0, 0, 0, 0, 0, 8}, /* 7 */
    {0, 0, 5, 0, 0, 0, 0, 0, 0, 0}, /* 8 */
    {0, 5, 0, 6, 0, 0, 0, 8, 0, 0}, /* 9 */
};

/* Node for the pattern tree */
struct tree_node {
    int id;
    struct tree_node *parent_node;
    struct tree_node *child_nodes[MAX_POINTS];
};

void init_subnode_list(struct tree_node *node);
void add_subnodes(struct tree_node *parent_node, const int level);
void delete_subtree(struct tree_node *node);
int illegal_transition(const int parent_id, const int child_id, 
                       const int branch_ids[], const int level);
void count_valid_patterns(const struct tree_node *const root, 
                          int pattern_count[], int level);

/*
 * Main function, program entry.
 */
int main(int argc, char *argv[])
{
    struct tree_node *root_node;
    int pattern_count[MAX_POINTS];
    int i;
    int sum = 0;

    /* currently no arguments are accepted */
    if (argc > 1)
        printf("Program %s does not accept arguments\n", argv[0]);

    /* init root node, not part of the unlock pattern */
    root_node = malloc(sizeof(struct tree_node));
    root_node->id = 0;
    root_node->parent_node = root_node;
    init_subnode_list(root_node);

    add_subnodes(root_node, 0);

    for (i = 0; i < MAX_POINTS; i++) {
        pattern_count[i] = 0;
    }

    count_valid_patterns(root_node, pattern_count, 0);

    for (i = 0; i < MAX_POINTS; i++) {
        printf("Number of patterns of lenght %d: %d\n", i+1, pattern_count[i]);
        sum += pattern_count[i];
    }

    printf("Number of all available patterns: %d\n", sum);

    /* clean up */
    delete_subtree(root_node);
    free(root_node);

    return EXIT_SUCCESS;
}

/*
 * Initialize subnode pointers to NULL
 *
 * \param node Node to initialize
 */
void init_subnode_list(struct tree_node *node)
{
    int i;

    for (i = 0; i < MAX_POINTS; i++) {
        node->child_nodes[i] = NULL;
    }

    return;
}

/*
 * Add all possible subnodes
 *
 * \param parent_node parent node to add subnodes to
 * \param level level of the tree we are on
 */
void add_subnodes(struct tree_node *parent_node, 
                  const int level)
{
    static int node_ids[MAX_POINTS]; /* keep track of the current branch */
    int child_count = 0;    /* number of children for this node */
    int i, j;

    for (i = 1; i <= MAX_POINTS; i++) {
        char present = 0;

        /* find if current node id is present on this branch */
        for (j = 0; j < level; j++) {
            if (i == node_ids[j]) {
                present = 1;
                break;
            }
        }

        if ((present > 0) || 
            (illegal_transition(parent_node->id, i, node_ids, level) > 0)) {
            continue;
        }

        /* save new node id */
        node_ids[level] = i;
        parent_node->child_nodes[child_count] =
            malloc(sizeof(struct tree_node));
        parent_node->child_nodes[child_count]->id = i;
        parent_node->child_nodes[child_count]->parent_node = parent_node;
        init_subnode_list(parent_node->child_nodes[child_count]);

        /* calculate subnodes for this child */
        add_subnodes(parent_node->child_nodes[child_count], level + 1);
        child_count++;

        /* branch for this node is done, so remove it */
        node_ids[level] = 0;
    }

    return;
}

/*
 * Delte entire subtree under the node
 *
 * \param node The node under which the subtree must be deleted.
 */
void delete_subtree(struct tree_node *node)
{
    int i;

    for (i = 0; (i < MAX_POINTS) && (node->child_nodes[i] != NULL); i++) {
        delete_subtree(node->child_nodes[i]);
        free(node->child_nodes[i]);
        node->child_nodes[i] = NULL;
    }

    return;
}

/*
 * Function to decide whether it is an illegal transition on the 
 * the current branch or not
 *
 * \param parent_id Node ID of the parent
 * \param child_id Node ID of the child
 * \param branch_ids list of nodes on this branch
 * \param level length of the branch
 * \return returns 1 if illegal transition, 0 if legal
 */
int illegal_transition(const int parent_id, 
                       const int child_id,
                       const int branch_ids[], 
                       const int level)
{
    const int blocker = block_matrix[parent_id][child_id];
    int i;

    /* legal transition if there is no blocker */
    if (blocker == 0) {
        return 0;
    }

    for (i = 0; i < level; i++) {
        /* if blocker node is already used then this transition is legal */
        if (blocker == branch_ids[i])
            return 0;
    }

    return 1;
}

/*
 * Count all valid android unlock patterns
 *
 * \param root root of the pattern tree to walk through
 * \param pattern_count array to store the count for each length of patterns
 * \param level level of the current tree
 */
void count_valid_patterns(const struct tree_node *const root,
                          int pattern_count[], 
                          int level)
{
    int i;

    for (i = 0; (i < MAX_POINTS) && (root->child_nodes[i] != NULL); i++) {
        pattern_count[level] = pattern_count[level] + 1;
        count_valid_patterns(root->child_nodes[i], pattern_count, level + 1);
    }

    return;
}

