//
//  minlib.c
//  Assignment5
//
//  Created by Robert Crosby on 6/4/13.
//  Copyright (c) 2013 Robert Crosby. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "minlib.h"


void openParts(data_t *data) {
   // primary partition
   if (data->part == -1)
      return;
   if (data->varbose)
      printf("\nPartition table:\n");
   openPart(data, data->part);
   
   // secondary partition
   if (data->sub == -1)
      return;
   if (data->varbose)
      printf("\nSubpartition table:\n");
   openPart(data, data->sub);
}

void openPart(data_t *data, int part) {
   uint8_t block[BLOCK_SIZE];
   ptable_t *ptable;
   
   fseek(data->file, data->start, SEEK_SET);
   fread((void*)block, BLOCK_SIZE, 1, data->file);
   
   if (block[510] != PMAGIC510 || block[511] != PMAGIC511) {
      printf("Invalid partition table.\n");
      printf("Unable to open disk image \"%s\".\n", data->image);
      closeImage(data);
      exit(3);
   }
   
   ptable = (ptable_t*)(block + PTABLE_OFFSET);
   if (data->varbose)
      printPTable(ptable);
   
   ptable += part;
   if (ptable->type != MINIX_PART) {
      printf("Not a Minix subpartition.\n");
      printf("Unable to open disk image \"%s\".\n", data->image);
      closeImage(data);
      exit(3);
   }
   data->start = ptable->lFirst * SECTOR_SIZE;
}

void readSuper(data_t *data) {
   uint8_t block[BLOCK_SIZE];
   supperblock_t *sblock;
   
   // read the supper block
   fseek(data->file, data->start+BLOCK_SIZE, SEEK_SET);
   fread((void*)block, BLOCK_SIZE, 1, data->file);
   sblock = (supperblock_t*)block;
   
   if (sblock->magic != MIN_MAGIC) {
      printf("Bad magic number. (0x%04x)\n", sblock->magic);
      printf("This doesn't look like a MINIX filesystem.\n");
      closeImage(data);
      exit(255);
   }
   
   data->zoneSize = sblock->blocksize << sblock->log_zone_size;
   data->sblock = *sblock;
   
   if (data->varbose)
      printSuper(&data->sblock);
}

void readINode(const data_t *data, inode_t *node, uint32_t num) {
   long offset = data->start + 2*data->sblock.blocksize;
   offset += data->sblock.blocksize*data->sblock.i_blocks;
   offset += data->sblock.blocksize*data->sblock.z_blocks;
   offset += (num - 1) * sizeof(inode_t);
   
   fseek(data->file, offset, SEEK_SET);
   fread((void*)node, sizeof(inode_t), 1, data->file);
}

uint32_t getINodeFromPath(const data_t *data, const char *path) {
   size_t size = strlen(path);
   uint32_t inode;
   
   if (size > 1) {
      char *copy = (char*)malloc(size);
      
      strcpy(copy, path+1);
      inode = getINode(data, copy, 1);
      free(copy);
   }
   else
      inode = 1;
   
   return inode;
}

uint32_t getINode(const data_t *data, char *path, uint32_t inode) {
   char name[NAME_SIZE+1], *next = strstr(path, "/");
   file_t *file = openFile(data, inode);
   entry_t *entry = file->entries;
   int i = 0, total = file->node.size/sizeof(entry_t);
   
   if (next)
      *next++ = '\0';
   name[NAME_SIZE] = '\0';
   
   for (i = 0; i < total; ++i, ++entry) {
      memcpy(name, entry->name, NAME_SIZE);
      if (!strcmp(path, name))
         break;
   }
   
   // not found
   if (i == total) {
      closeFile(file);
      return 0;
   }
   
   // copy the inode
   inode = entry->inode;
   closeFile(file);
   
   // not at end of path
   if (next && *next)
      return getINode(data, next, inode);
   
   return inode;
}

file_t* openFileFromPath(const data_t *data, const char *path) {
   file_t *file = (file_t*)malloc(sizeof(file_t));
   uint32_t inode;
   
   if (path[0] != '/') {
      file->path = (char*)malloc(strlen(path) + 1);
      file->path[0] = '/';
      strcpy(file->path+1, path);
   }
   else {
      file->path = (char*)malloc(strlen(path));
      strcpy(file->path, path);
   }
   
   inode = getINodeFromPath(data, file->path);
   if (!inode) {
      free(file);
      return NULL;
   }
   readINode(data, &file->node, inode);
   file->contents = NULL;
   file->entries = NULL;
   file->numEntries = 0;
   
   if (MIN_ISDIR(file->node.mode))
      loadDir(data, file);
   
   return file;
}

file_t* openFile(const data_t *data, uint32_t node) {
   file_t *file = (file_t*)malloc(sizeof(file_t));
   
   readINode(data, &file->node, node);
   file->contents = NULL;
   file->entries = NULL;
   file->numEntries = 0;
   file->path = NULL;
   
   if (MIN_ISDIR(file->node.mode))
      loadDir(data, file);
   return file;
}

void loadFile(const data_t *data, file_t *file) {
   long size = file->node.size;
   
   if (size) {
      int i;
      uint8_t *cur, *buff;
      
      buff = (uint8_t*)malloc(data->zoneSize);
      file->contents = (uint8_t*)malloc(size);
      cur = file->contents;
      
      for (i = 0; i < DIRECT_ZONES && size > 0; ++i) {
         long transfer = size < data->zoneSize ? size : data->zoneSize;
         
         if (file->node.zone[i]) {
            long offset = data->start + data->zoneSize * file->node.zone[i];
            
            fseek(data->file, offset, SEEK_SET);
            fread((void*)buff, transfer, 1, data->file);
            memcpy(cur, buff, transfer);
         }
         else
            memset(cur, 0, transfer);
         
         size -= data->zoneSize;
         cur += data->zoneSize;
      }
      
      free(buff);
   }
   else
      file->contents = NULL;
}

void loadDir(const data_t *data, file_t *dir) {
   int links;
   
   loadFile(data, dir);
   links = dir->node.links + 2;
   
   if (dir->contents) {
      entry_t *cur, *entry = (entry_t*)dir->contents;
      int i;
      
      dir->numEntries = dir->node.size/sizeof(entry_t);
      dir->entries = (entry_t*)malloc(dir->numEntries * sizeof(entry_t));
      for (i = 0, cur = dir->entries; i < dir->numEntries; ++i, ++cur, ++entry)
         *cur = *entry;
   }
}

void closeFile(file_t *file) {
   free(file->contents);
   free(file->entries);
   free(file);
}

void printPTable(ptable_t *part) {
   int i;
   
   printf("       ----Start----      ------End-----\n");
   printf("  Boot head  sec  cyl Type head  sec  cyl      First       Size\n");
   
   for (i = 0; i < 4; ++i, ++part) {
      printf("  0x%02x ", part->bootind);
      printf("%4u ", part->start_head);
      printf("%4u ", part->start_sec);
      printf("%4u ", part->start_cyl);
      printf("0x%02x ", part->type);
      printf("%4u ", part->end_head);
      printf("%4u ", part->end_sec);
      printf("%4u ", part->end_cyl);
      printf("%10u ", part->lFirst);
      printf("%10u\n", part->size);
   }
}

void printSuper(const supperblock_t *sblock) {
   uint32_t zoneSize = sblock->blocksize << sblock->log_zone_size;
   
   printf("\nSuperblock Contents:\n");
   printf("Stored Fields:\n");
   printf("  ninodes %12u\n", sblock->ninodes);
   printf("  i_blocks %11d\n", sblock->i_blocks);
   printf("  z_blocks %11d\n", sblock->z_blocks);
   printf("  firstdata %10u\n", sblock->firstdata);
   printf("  log_zone_size %6d", sblock->log_zone_size);
   printf(" (zone size: %u)\n", zoneSize);
   printf("  max_file %11u\n", sblock->max_file);
   printf("  magic         0x%04x\n", sblock->magic);
   printf("  zones %14u\n", sblock->zones);
   printf("  blocksize %10u\n", sblock->blocksize);
   printf("  subversion %9u\n", sblock->subversion);
   
   printf("Computed Fields:\n");
   printf("  firstIblock %8u\n", 4);      //fix
   printf("  zonesize %11u\n", zoneSize);
   printf("  ptrs_per_zone %6u\n", 1024); //fix
   printf("  ino_per_block %6u\n", 64);   //fix
   printf("  wrongended %9u\n\n", sblock->magic == MIN_MAGIC_REV);
}

void printINode(const inode_t *node) {
   char tbuff[NAME_SIZE];
   time_t time;
   int i;
   
   printf("File inode:\n");
   printf("  unsigned short mode         0x%04x", node->mode);
   printf("\t(");
   printMode(node->mode);
   printf(")\n");
   
   printf("  unsigned short links %13u\n", node->links);
   printf("  unsigned short uid %15u\n", node->uid);
   printf("  unsigned short gid %15u\n", node->gid);
   printf("  unsigned long  size %14u\n", node->size);
   
   time = node->atime;
   strftime(tbuff, NAME_SIZE, "%a %b %e %T %Y", localtime(&time));
   printf("  unsigned long  atime %13u\t--- %s\n", node->atime, tbuff);
   
   time = node->mtime;
   strftime(tbuff, NAME_SIZE, "%a %b %e %T %Y", localtime(&time));
   printf("  unsigned long  mtime %13u\t--- %s\n", node->mtime, tbuff);
   
   time = node->ctime;
   strftime(tbuff, NAME_SIZE, "%a %b %e %T %Y", localtime(&time));
   printf("  unsigned long  ctime %13u\t--- %s\n", node->ctime, tbuff);
   
   printf("\n  Direct zones:\n");
   for (i = 0; i < DIRECT_ZONES; ++i)
      printf("              zone[%d]   = %10u\n", i, node->zone[i]);
   
   printf("  unsigned long  indirect %10u\n", node->indirect);
   printf("  unsigned long  double %12u\n", node->unused);
}

void printMode(uint16_t mode) {
   char *perm, perms[PERM_SIZE];
   
   // build permission string
   perm = perms;
   *perm++ = MIN_ISDIR(mode)  ? 'd' : '-';
   *perm++ = mode & MIN_IRUSR ? 'r' : '-';
   *perm++ = mode & MIN_IWUSR ? 'w' : '-';
   *perm++ = mode & MIN_IXUSR ? 'x' : '-';
   *perm++ = mode & MIN_IRGRP ? 'r' : '-';
   *perm++ = mode & MIN_IWGRP ? 'w' : '-';
   *perm++ = mode & MIN_IXGRP ? 'x' : '-';
   *perm++ = mode & MIN_IROTH ? 'r' : '-';
   *perm++ = mode & MIN_IWOTH ? 'w' : '-';
   *perm++ = mode & MIN_IXOTH ? 'x' : '-';
   *perm = '\0';
   
   printf("%s", perms);
}

void openImage(data_t *data) {
   data->file = fopen(data->image, "rb");
   
   if (!data->file) {
      printf("Unable to open disk image \"%s\".\n", data->image);
      exit(3);
   }
}

void closeImage(data_t *data) {
   fclose(data->file);
}