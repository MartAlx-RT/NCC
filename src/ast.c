#include "ncc.h"

node_t *NewNode(const node_data_t data)
{
	node_t *new_node = (node_t *)calloc(1, sizeof(node_t));
	assert(new_node);

	new_node->data = data;

	return new_node;
}

void AddChild(node_t *node, node_t *new_child)
{
	assert(node);
	assert(new_child);

	new_child->parent = node;

	if(node->child == NULL)
	{
		node->child = (child_t *)calloc(1, sizeof(child_t));
		assert(node->child);

		node->child->node = new_child;
	}
	else
	{
		child_t *child = node->child;
		while (child->next)
			child = child->next;

		child->next = (child_t *)calloc(1, sizeof(child_t));
		assert(child->next);

		child->next->node = new_child;
		child->next->prev = child;
	}
}

void TreeDestroy(node_t *tree)
{
	if(tree == NULL)
		return;

	if(tree->child)
	{
		child_t *child = tree->child;
		child_t *prev = NULL;
		while (child)
		{
			//free(child->prev);
			child->prev = NULL;
			
			TreeDestroy(child->node);
			child->node = NULL;
			prev = child;
			child = child->next;
			free(prev);
		}
	}

	free(tree);
}


