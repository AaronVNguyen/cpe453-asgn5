//
//  main.c
//  minls
//
//  Created by Robert Crosby on 6/4/13.
//  Copyright (c) 2013 Robert Crosby. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "minlib.h"

void parseArgs(data_t *data, int argc, const char * argv[], const char *app);
void printHelp();
void printListing(data_t *data);
void printEntry(const data_t *data, const entry_t *entry);

int main(int argc, const char * argv[]) {
	const char *app = "minls";
	data_t data;

	parseArgs(&data, argc, argv, app);
	openImage(&data);
	openParts(&data);
	readSuper(&data);
	printListing(&data);

	closeImage(&data);

	return 0;
}

void parseArgs(data_t *data, int argc, const char * argv[], const char *app) {
	int i;
	const char *defPath = "/";

	data->path = defPath;
	data->image = NULL;
	data->part = -1;
	data->sub = -1;
	data->varbose = 0;
	data->start = 0;

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-p") && ++i < argc)
			sscanf(argv[i], " %i", &data->part);
		else if (!strcmp(argv[i], "-s") && ++i < argc)
			sscanf(argv[i], " %i", &data->sub);
		else if (!strcmp(argv[i], "-v"))
			data->varbose = 1;
		else if (!strcmp(argv[i], "-h")) {
			printHelp();
			exit(1);
		} else if (!data->image)
			data->image = argv[i];
		else
			data->path = argv[i];
	}

	if (!data->image) {
		printHelp();
		exit(1);
	}
}

void printHelp() {
	printf("usage: minls [ -v ] [ -p num [ -s num ] ] imagefile [ path ]\n");
	printf("Options:\n");
	printf(
			"\t-p\tpart    --- select partition for filesystem (default: none)\n");
	printf(
			"\t-s\tsub     --- select subpartition for filesystem (default: none)\n");
	printf("\t-h\thelp    --- print usage information and exit\n");
	printf("\t-v\tverbose --- increase verbosity level\n");
}

void printListing(data_t *data) {
	file_t *file;

	file = openFileFromPath(data, data->path);

	if (file) {
		if (data->varbose)
			printINode(&file->node);

		if (file->entries) {
			printf("%s:\n", data->path);
			entry_t *entry = file->entries;
			int i;

			for (i = 0; i < file->numEntries; ++i, ++entry) {
				if (entry->inode)
					printEntry(data, entry);
			}
		} else {
			printMode(file->node.mode);
			printf("%10d %s\n", file->node.size, file->path);
		}
		closeFile(file);
	} else {
		printf("%s: File not found.\n", data->path);
		closeImage(data);
		exit(1);
	}
}

void printEntry(const data_t *data, const entry_t *entry) {
	char *name = (char*) malloc(NAME_SIZE + 1);
	inode_t node;

	// get the inode
	readINode(data, &node, entry->inode);

	// copy the name
	memcpy(name, entry->name, NAME_SIZE);
	name[NAME_SIZE] = '\0';

	// print the entry
	printMode(node.mode);
	printf("%10d %s\n", node.size, name);
	free(name);
}
