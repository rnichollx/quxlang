#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/** One allocation represents each node, including leaves. */
struct tree_node
{
    struct tree_node* left;
    struct tree_node* right;
};

/** Recursively allocates a perfect binary tree. */
struct tree_node* allocate_tree(unsigned depth)
{
    struct tree_node* node = malloc(sizeof(*node));
    if (!node)
    {
        abort();
    }
    node->left = depth ? allocate_tree(depth - 1) : NULL;
    node->right = depth ? allocate_tree(depth - 1) : NULL;
    return node;
}

/** Counts every node in a previously allocated tree. */
uint64_t check_tree(const struct tree_node* node)
{
    return node->left ? 1 + check_tree(node->left) + check_tree(node->right) : 1;
}

/** Recursively deallocates every node. */
void delete_tree(struct tree_node* node)
{
    if (node->left)
    {
        delete_tree(node->left);
        delete_tree(node->right);
    }
    free(node);
}

/** Runs the stretch, temporary, and long-lived tree workloads. */
int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    int requested = atoi(argv[1]);
    if (requested < 0 || requested > 24)
    {
        return 2;
    }
    unsigned maximum = requested < 6 ? 6 : (unsigned)requested;
    struct tree_node* stretch = allocate_tree(maximum + 1);
    printf("stretch tree of depth %u\t check: %" PRIu64 "\n", maximum + 1, check_tree(stretch));
    delete_tree(stretch);
    struct tree_node* long_lived = allocate_tree(maximum);
    for (unsigned depth = 4; depth <= maximum; depth += 2)
    {
        uint64_t iterations = UINT64_C(1) << (maximum - depth + 4);
        uint64_t check = 0;
        for (uint64_t i = 0; i < iterations; ++i)
        {
            struct tree_node* tree = allocate_tree(depth);
            check += check_tree(tree);
            delete_tree(tree);
        }
        printf("%" PRIu64 "\t trees of depth %u\t check: %" PRIu64 "\n", iterations, depth, check);
    }
    printf("long lived tree of depth %u\t check: %" PRIu64 "\n", maximum, check_tree(long_lived));
    delete_tree(long_lived);
    return 0;
}
