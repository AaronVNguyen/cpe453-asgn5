#ifndef Assignment5_minlib_h
#define Assignment5_minlib_h

#include <stdint.h>

#define SECTOR_SIZE 512
#define BLOCK_SIZE 1024

#define PTABLE_OFFSET 0x1be
#define PMAGIC510 0x55
#define PMAGIC511 0xaa
#define MINIX_PART 0x81
#define BOOT_MAGIC  0x80

#define MIN_MAGIC 0x4d5a
#define MIN_MAGIC_REV 0x5a4d
#define MIN_MAGIC_OLD 0x2468
#define MIN_MAGIC_OLD_REV 0x6824

#define MIN_ISREG(m) (((m)&0170000)==0100000)
#define MIN_ISDIR(m) (((m)&0170000)==0040000)
#define MIN_IRUSR 0400
#define MIN_IWUSR 0200
#define MIN_IXUSR 0100
#define MIN_IRGRP 0040
#define MIN_IWGRP 0020
#define MIN_IXGRP 0010
#define MIN_IROTH 0004
#define MIN_IWOTH 0002
#define MIN_IXOTH 0001

#define DIRECT_ZONES 7
#define NAME_SIZE 60
#define PERM_SIZE 10

typedef struct ptable {
	uint8_t bootind; /* 0x80 for bootable */
	uint8_t start_head;
	uint8_t start_sec;
	uint8_t start_cyl;
	uint8_t type; /* 0x81 for MINIX */
	uint8_t end_head;
	uint8_t end_sec;
	uint8_t end_cyl;
	uint32_t lFirst; /* first sector */
	uint32_t size; /* size of partition in sectors */
} ptable_t;

typedef struct supperblock {
	uint32_t ninodes; /* inodes in system */
	uint16_t pad1;
	int16_t i_blocks; /* blocks used in inode bit map */
	int16_t z_blocks; /* blocks used in zone bit map */
	uint16_t firstdata;
	int16_t log_zone_size; /* log2 of blocks per zone */
	int16_t pad2;
	uint32_t max_file; /* max file size */
	uint32_t zones; /* number of zones on disk */
	int16_t magic; /* 0x4d5a */
	int16_t pad3;
	uint16_t blocksize; /* block size in bytes */
	uint8_t subversion;
} supperblock_t;

typedef struct inode {
	uint16_t mode;
	uint16_t links;
	uint16_t uid;
	uint16_t gid;
	uint32_t size;
	int32_t atime;
	int32_t mtime;
	int32_t ctime;
	uint32_t zone[DIRECT_ZONES];
	uint32_t indirect;
	uint32_t two_indirect;
	uint32_t unused;
} inode_t;

typedef struct entry {
	uint32_t inode;
	uint8_t name[NAME_SIZE];
} entry_t;

typedef struct file {
	inode_t node;
	entry_t entry;
	uint8_t *contents;
	entry_t *entries;
	size_t numEntries;
	char *path;
} file_t;

typedef struct data {
	const char *image;
	const char *path;
	int part;
	int sub;
	int varbose;
	FILE *file;

	long start;
	long zoneSize;
	supperblock_t sblock;
} data_t;

//void parseArgs(data_t *data, int argc, const char * argv[], const char *app);
void openParts(data_t *data);
void openPart(data_t *data, int part);
void readSuper(data_t *data);

void readINode(const data_t *data, inode_t *node, uint32_t num);

uint32_t getINodeFromPath(const data_t *data, const char *path);
uint32_t getINode(const data_t *data, char *name, uint32_t inode);

file_t* openFileFromPath(const data_t *data, const char *path);
file_t* openFile(const data_t *data, uint32_t node);

void loadFile(const data_t *data, file_t *file);
void loadDir(const data_t *data, file_t *dir);
void closeFile(file_t *file);

void printPTable(ptable_t *part);
void printSuper(const supperblock_t *sblock);
void printINode(const inode_t *node);
void printMode(uint16_t mode);

void openImage(data_t *data);
void closeImage(data_t *data);

#endif
