/*
 * Andorid unlock pattern calculator.
 * Copyright (c) 2011  Zoltan Puskas
 *  All rights reserved.
 *
 * This program is free software and redistributred under the 3-clause BSD
 * license as stated below.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 * - print valid patterns if dots are given (try to guess the probale one?)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/* Number of points in the pattern which 
 * (it is also the maximum depth for the tree) */
#define MAX_POINTS 9

/* Node for the pattern tree */
struct tree_node {
    int id;
    int child_count;
    struct tree_node *parent_node;
    struct tree_node *child_nodes[MAX_POINTS];
};

/* Matrix describing which transition is blocked by which node */
int pattern_block_matrix[10][10] = {
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

/* Matrix describing which transition is blocked by which node for guessing */
int guess_matrix[10][10];

void print_help(const char* argv0);
void init_subnode_list(struct tree_node *node);
void add_subnodes(struct tree_node *parent_node, const int level, 
                  int block_matrix[][10]);
void delete_subtree(struct tree_node *node);
int illegal_transition(const int parent_id, const int child_id, 
                       const int branch_ids[], const int level,
                       int block_matrix[][10]);
void count_valid_patterns(const struct tree_node *const root, 
                          int pattern_count[], const int level);
void subtree_to_file(const struct tree_node * const node, 
                     FILE* const output_file);
void print_summary(const struct tree_node * const root_node);
void print_random_patterns(const struct tree_node * const root_node, int len);
void fill_guess_matrix(char* nodelist, int block_matrix[][10]);

/*
 * Main function, program entry.
 */
int main(int argc, char *argv[])
{
    struct tree_node *root_node;
    struct tree_node *guess_root_node;
    int opt;
    int summary_flag = 0;
    int guess_flag = 0;
    int gen_pattern_len = 0;
    FILE *pattern_file = NULL;
    char *guess_node_list;

    if (argc < 2) {
        fprintf(stderr, "No arguments specified.\n");
        print_help(argv[0]);
    }

    /* currently no arguments are accepted */
    while((opt = getopt(argc, argv, "sr:o:g:e:h")) != -1) {
        switch (opt) {
        case 's':
            summary_flag = 1;
            break;
        case 'r':
            if(atoi(optarg) > 0) {
                gen_pattern_len = atoi(optarg);
            } else {
                fprintf(stderr, "Invalid parameter %s for -r flag!", optarg);
            }
            break;
        case 'o':
            pattern_file = fopen(optarg, "w");
            if (pattern_file == NULL) {
                fprintf(stderr, 
                "Could not open \"%s\" output file for writting",
                optarg);
            }
            break;
        case 'g':
            guess_flag = 1;
            fill_guess_matrix(optarg, guess_matrix);
            guess_node_list = optarg;
            break;
        case 'e':
            printf("Edge: %d\n", atoi(optarg));
            break;
        case 'h':
        default:
            print_help(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (summary_flag > 0 || pattern_file != NULL  || gen_pattern_len > 0) {
        /* init root node, not part of the unlock pattern */
        root_node = malloc(sizeof(struct tree_node));
        root_node->id = 0;
        root_node->parent_node = root_node;
        init_subnode_list(root_node);

        /* build the entire valid pattern tree */
        add_subnodes(root_node, 0, pattern_block_matrix);

        if (summary_flag > 0) {
            print_summary(root_node);
            if (pattern_file != NULL) {
                fprintf(pattern_file, "Patterns based on all nodes\n");
                subtree_to_file(root_node, pattern_file);
            }
        }

        if (gen_pattern_len > 0) {
            print_random_patterns(root_node, gen_pattern_len);
        }

        /* clean up */
        delete_subtree(root_node);
        free(root_node);
    }

    if(guess_flag > 0) {
        /* init root node, not part of the unlock pattern */
        guess_root_node = malloc(sizeof(struct tree_node));
        guess_root_node->id = 0;
        guess_root_node->parent_node = guess_root_node;
        init_subnode_list(guess_root_node);

        /* build the valid pattern tree based on available nodes */
        add_subnodes(guess_root_node, 0, guess_matrix);

        /* print the summary */
        print_summary(guess_root_node);

        if (pattern_file != NULL) {
            fprintf(pattern_file, "Guessed patterns based on nodes: %s\n",
                    guess_node_list);
            subtree_to_file(guess_root_node, pattern_file);
        }

        /* clean up */
        delete_subtree(guess_root_node);
        free(guess_root_node);
    }

    if (pattern_file != NULL) {
        fclose(pattern_file);
    }
    return EXIT_SUCCESS;
}
/*
 * Print help and usage information for the user.
 */
void print_help(const char* argv0)
{
    fprintf(stderr,
            "Usage: %s [-s] [-r LENGTH] [-o FILE] [-g NODES] [-e EDGE] [-h]\n",
            argv0);
    fprintf(stderr, "\n");
    fprintf(stderr,
            "   -s\tPrint summary on all patterns.\n");
    fprintf(stderr,
            "   -r\tGenerate random unlock patterns with given LENGTH.\n");
    fprintf(stderr,
            "   -o\tOutput patterns to file. Can be used with -s and -g.\n");
    fprintf(stderr,
            "   -g\tGuess patterns based on the NODES. (eg.: 73652)\n");
    fprintf(stderr,
            "   -e\tEdge not to include while guessing. (eg.: 12)\n");
    fprintf(stderr,
            "   -h\tPrint this help message.\n");

    return;
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
        node->child_count = 0;
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
                  const int level, int block_matrix[][10])
{
    static int node_ids[MAX_POINTS]; /* keep track of the current branch */
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
            (illegal_transition(parent_node->id, i, node_ids, 
                                level, block_matrix) > 0))
        {
            continue;
        }

        /* save new node id */
        node_ids[level] = i;
        parent_node->child_nodes[parent_node->child_count] =
            malloc(sizeof(struct tree_node));
        parent_node->child_nodes[parent_node->child_count]->id = i;
        parent_node->child_nodes[parent_node->child_count]->parent_node =
            parent_node;
        init_subnode_list(parent_node->child_nodes[parent_node->child_count]);

        /* calculate subnodes for this child */
        add_subnodes(parent_node->child_nodes[parent_node->child_count],
                     level + 1, block_matrix);
        parent_node->child_count++;

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

    for (i = 0; i < node->child_count; i++) {
        delete_subtree(node->child_nodes[i]);
        free(node->child_nodes[i]);
        node->child_nodes[i] = NULL;
    }

    node->child_count = 0;

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
                       const int level,
                       int block_matrix[][10])
{
    int blocker = block_matrix[parent_id][child_id];
    int i;

    /* legal transition if there is no blocker */
    if (blocker == 0) {
        return 0;
    }

    /* illegal transition because it is disabled */
    if (blocker == -1)
    {
        return 1;
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
                          const int level)
{
    int i;

    for (i = 0; i < root->child_count; i++) {
        pattern_count[level] = pattern_count[level] + 1;
        count_valid_patterns(root->child_nodes[i], pattern_count, level + 1);
    }

    return;
}

/*
 * Print (sub)patterns to file
 */
void subtree_to_file(const struct tree_node * const node, 
                     FILE* const output_file)
{
    static int branch;
    int i;

    if (branch == 0) {
        branch = node->id;
    }

    for(i = 0; i < node->child_count; i++) {
        fprintf(output_file, "%d\n", (branch * 10) + node->child_nodes[i]->id);
    }

    for(i = 0; i < node->child_count; i++) {
        branch = branch * 10 + node->child_nodes[i]->id;
        subtree_to_file(node->child_nodes[i], output_file);
        branch = branch / 10;
    }

    return;
}

/*
 * Print summary of available patterns
 *
 * \param root_node Root node of the pattern tree
 */
void print_summary(const struct tree_node * const root_node)
{
    int pattern_count[MAX_POINTS];
    int sum = 0;
    int valid_sum = 0;
    int i;

    for (i = 0; i < MAX_POINTS; i++) {
        pattern_count[i] = 0;
    }

    count_valid_patterns(root_node, pattern_count, 0);

    for (i = 0; i < MAX_POINTS; i++) {
        printf("Number of patterns for lenght %d: %d\t\
                Minutes to bruteforce*: %d\n",
                i+1, pattern_count[i], pattern_count[i]/5);
        sum += pattern_count[i];
        if (i > 2) valid_sum += pattern_count[i];
    }
    printf("-------------------------------------------\n");
    printf("Number of all available patterns: %d\n", sum);
    printf("Number of valid patterns (length >= 4): %d\n", valid_sum);
    printf("(* assuming 5 tries in 30 seconds and then a 30 second timeout)\n");

    return;
}

/*
 * Print 10 random unlock patterns of specified length
 *
 * \param root_node Root node of the pattern tree
 * \param len length of the patterns to print (minimum 4)
 */
void print_random_patterns(const struct tree_node * const root_node, int len)
{
    int i;
    int pattern_len;
    const struct tree_node * current_node = root_node;
    
    if ((len < 4) || (len > 9)) {
        fprintf(stderr, "%d is invalid pattern length. Must be 4-9!\n", len);
    }

    srand(time(NULL));

    for(i = 0; i < 10; i++) {
        pattern_len = 0;
        current_node = root_node;
        while (pattern_len < len) {
            int dot = (int)(rand() % current_node->child_count);
            printf("%d", current_node->child_nodes[dot]->id);
            current_node = current_node->child_nodes[dot];
            pattern_len++;
        }
        printf("\n");
    }

    return;
}

/*
 * Fill up the restricted transition matrix with the limited transitions
 *
 * \param nodelist node numbers
 * \param block_matrix the transion matrix to set up
 */
void fill_guess_matrix(char* nodelist, int block_matrix[][10])
{
    int nodes[MAX_POINTS] = {0};
    int i,j;
    
    /* init all transitions to illegal */
    for(i=0; i<MAX_POINTS+1; i++) {
        for(j=0; j<10; j++) {
            block_matrix[i][j] = -1;
        }
    }

    /* add nodes to be used of guessing */
    for(i=0, j=0; nodelist[i]!='\0'; i++)
    {
        j = nodelist[i] - '0';
        if((j >= 0) && (j <= MAX_POINTS)) {
            nodes[j] = j;
        }
    }

    /* copy valid parts from the original transition matrix */
    for(i = 0; i < MAX_POINTS+1; i++)
    {
        for(j = 0; j < MAX_POINTS+1; j++)
        {
            block_matrix[nodes[i]][nodes[j]] =
                    pattern_block_matrix[nodes[i]][nodes[j]];
        }
    }
    
    return;
}

