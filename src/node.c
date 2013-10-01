/* node.c
 * ls-fuse - ls -lR output mounter
 *
 * Copyright (C) 2013 Dmitry Podgorny <pasis.ua@gmail.com>
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
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"
#include "tools.h"

lsnode_t root = {
	.mode = S_IFDIR | 0755,
	.name = "/",
};

/* XXX: temporary solution */
extern lsnode_t *cwd;

lsnode_t *node_alloc(void)
{
	lsnode_t *node = malloc(sizeof(lsnode_t));
	if (!node) {
		return NULL;
	}

	memset(node, 0, sizeof(lsnode_t));
	node->next = cwd->entry;
	cwd->entry = node;

	return node;
}

void node_free(lsnode_t *node)
{
	(void)node;
	/* TODO */
}

/* node_create_data must be thread safe */
void node_create_data(lsnode_t *node)
{
	static const char data[] = "File: %s\n"
				   "Size: %s\n"
				   "Mode: %s\n"
				   "Owner: %s\n"
				   "SELinux context: %s\n";
	static const char units[] = {'\0', 'K', 'M', 'G', 'T', 'P'};

	char *mode;
	char *owner;
	char *selinux;
	char size[8];
	size_t n;
	size_t i;
	off_t cut;

	if (!node->name) {
		return;
	}

	/* TODO: implement mode and owner */
	selinux = owner = mode = "";
	if (node->selinux != NULL) {
		selinux = node->selinux;
	}
	
	cut = node->size;
	i = 0;
	while (cut >= 10000 && i < ARRAY_SIZE(units)) {
		cut /= 1024;
		i++;
	}
	snprintf(size, sizeof(size), "%d%c", (int)cut, units[i]);

	n = strlen(node->name) + strlen(size) + strlen(mode) + strlen(owner) +
	    strlen(selinux) + sizeof(data) - 10;

	node->data = malloc(n);
	if (node->data != NULL) {
		snprintf(node->data, n, data, node->name, size, mode, owner,
			 selinux);
	}
}

/* node_from_path must be thread safe */
lsnode_t *node_from_path(const char * const path)
{
	lsnode_t *parent;
	lsnode_t *node;
	char *tmp = strdup(path);
	char *tok;
	char *saveptr = NULL;
	int found;

	parent = &root;
	tok = strtok_r(tmp, "/", &saveptr);

	while (tok) {
		if (strcmp(tok, ".") == 0) {
			/* do nothing, parent remains the same */
		} else if (strcmp(tok, "..") == 0) {
			/* TODO: not implemented yet (doubly linked list?) */
		} else {
			node = parent->entry;
			found = FALSE;
			while (node) {
				if (node->name && !strcmp(tok, node->name)) {
					found = TRUE;
					parent = node;
					break;
				}
				node = node->next;
			}

			if (!found) {
				parent = NULL;
				break;
			}
		}

		tok = strtok_r(NULL, "/", &saveptr);
	}

	free(tmp);

	return parent;
}
