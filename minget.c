#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "minlib.h"

void parseArgs(data_t *data, int argc, const char * argv[]);
void copyFile(data_t *data);
void printHelp();

int main(int argc, const char *argv[]) {
	data_t data;

	parseArgs(&data, argc, argv);
	openImage(&data);
	openParts(&data);
	readSuper(&data);
	copyFile(&data);

	closeImage(&data);

	return 0;
}

void parseArgs(data_t *data, int argc, const char * argv[]) {
	int i;

	data->path = NULL;
	data->image = NULL;
	data->host = NULL;
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
		else if (!data->path)
			data->path = argv[i];
		else
			data->host = argv[i];
	}

	if (!data->image || !data->path) {
		printHelp();
		exit(1);
	}
}

void copyFile(data_t *data) {
	file_t *file;
   
   file = openFileFromPath(data, data->path);
   
   if (file) {
      if (MIN_ISDIR(file->node.mode)) {
         printf("%s: Not a regular file.\n", data->path);
         closeImage(data);
         exit(1);
      }
      
      loadFile(data, file);
      if (data->host) { // write to a file
         FILE *fout = fopen(data->host, "wb");
         
         if (!fout) {
            printf("Could not open outfile\n");
            closeFile(file);
            closeImage(data);
            exit(1);
         }
         
         fwrite(file->contents, file->node.size, 1, fout);
         fclose(fout);
//         printf("write to file %s\n", data->host);
      }
      else {
         // write to console
         write(1, file->contents, file->node.size);
      }
      
      closeFile(file);
   }
   else {
      printf("%s: File not found.\n", data->path);
      closeImage(data);
      exit(1);
   }
}

void printHelp() {
	printf("usage: minget  [ -v ] [ -p num [ -s num ] ] "
			"imagefile minixpath [ hostpath ]\n");
	printf("Options:\n");
	printf("\t-p\t part    --- select partition for "
			"filesystem (default: none)\n");
	printf("\t-s\t sub     --- select subpartition for "
			"filesystem (default: none)\n");
	printf("\t-h\t help    --- print usage information and exit\n");
	printf("\t-v\t verbose --- increase verbosity level\n");
}
