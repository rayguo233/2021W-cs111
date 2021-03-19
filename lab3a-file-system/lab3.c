#include <stdlib.h> // for exit
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h> // for strerror
#include <unistd.h>
#include <stdint.h>
#include "ext2_fs.h"
#include <time.h>

#define TIME_STR_LEN 51

int fd;
unsigned int block_size;
unsigned int ptr_per_block;
struct ext2_group_desc groupd;

void get_time(const time_t time, char* str);
unsigned int get_offset(unsigned int nblock);
void read_inode(unsigned int index);
void read_dinode(int level, unsigned int parent, unsigned int nblock);
void read_indir_block(unsigned int level_below, 
	unsigned int inode_num, unsigned int offset, unsigned int nblock);
ssize_t mypread(int fd, void *buf, size_t count, off_t offset);

int main(int argc, char **argv) {
    // open file
    if (argc != 2) {
    	fprintf(stderr, "ERROR: wrong number of arguments. Usage 'lab3a IMAGE'\n");
    	exit(1);
    }
	unsigned int inodes_count, blocks_count, first_ino,
		blocks_per_group, inodes_per_group, inode_size;
	if ((fd = open(argv[1], O_RDONLY)) == -1) {
    	fprintf(stderr, "ERROR: 'open()' - %s\n", strerror(errno));
		exit(1);
	}

	// superblock
	struct ext2_super_block super;
	mypread(fd, &super, sizeof(super), 1024);
	inodes_count = super.s_inodes_count;
	blocks_count = super.s_blocks_count;
	block_size = 1024 << super.s_log_block_size;
	ptr_per_block = block_size / 4;
	inode_size = super.s_inode_size;
	blocks_per_group = super.s_blocks_per_group;
	inodes_per_group = super.s_inodes_per_group;
	first_ino = super.s_first_ino;
	printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, 
		block_size, inode_size, blocks_per_group, inodes_per_group, first_ino);

	// group summary (Assuming only 1 group)
	unsigned int bg_free_blocks_count, bg_free_inodes_count, bg_block_bitmap,
		bg_inode_bitmap, offset, bg_inode_table;
	if (block_size == 1024) offset = 2048;
	else offset = block_size;
	mypread(fd, &groupd, sizeof(groupd), offset);
	bg_free_blocks_count = groupd.bg_free_blocks_count;
	bg_free_inodes_count = groupd.bg_free_inodes_count;
	bg_block_bitmap = groupd.bg_block_bitmap;
	bg_inode_bitmap = groupd.bg_inode_bitmap;
	bg_inode_table = groupd.bg_inode_table;
	printf("GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, 
		bg_free_blocks_count, bg_free_inodes_count, bg_block_bitmap,
		bg_inode_bitmap, bg_inode_table);

	// free block entries
	char* bitmap;
	bitmap = malloc(block_size);
	mypread(fd, bitmap, block_size, block_size * bg_block_bitmap);
	for (unsigned int i = 0; i < blocks_count - 1; i++) {
		unsigned int index, offset;
		index = i / 8;
		offset = i % 8;
		if ((bitmap[index] & (1<<offset)) == 0) {
			printf("BFREE,%u\n", i + 1);
		}
	}

	// I-node + directory + indirect entries
	mypread(fd, bitmap, block_size, block_size * bg_inode_bitmap);
	for (unsigned int i = 0; i < inodes_count; i++) {
		unsigned int index, offset;
		index = i / 8;
		offset = i % 8;
		if (bitmap[index] & (1<<offset))
			read_inode(i+1);
		else 
			printf("IFREE,%u\n", i + 1);
	}
	free(bitmap);

	return 0;
}

void read_inode(unsigned int index) {
	struct ext2_inode inode;
	mypread(fd, &inode, sizeof(inode), 
		groupd.bg_inode_table * block_size + index * sizeof(inode));
	if (inode.i_mode == 0 || inode.i_links_count == 0) 
		return;
	char ftype = '?';
	if (S_ISREG(inode.i_mode)) ftype = 'f';
	else if (S_ISDIR(inode.i_mode)) ftype = 'd';
	else if (S_ISLNK(inode.i_mode)) ftype = 's';
	char mtime[TIME_STR_LEN], ctime[TIME_STR_LEN], atime[TIME_STR_LEN];
	get_time(inode.i_mtime, mtime);
	get_time(inode.i_ctime, ctime);
	get_time(inode.i_atime, atime);
	printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", index+1, ftype, 
		inode.i_mode & 0xFFF, inode.i_uid, inode.i_gid, 
		inode.i_links_count, ctime, mtime, atime, inode.i_size,
		inode.i_blocks);
	if (ftype == 'f' || ftype == 'd' || (ftype == 's' && inode.i_size >= 60)) {
		for (int i = 0; i < 15; i++) {
			printf(",%u", inode.i_block[i]);
		}
	} 
	else if (ftype == 's') {
		printf(",%u", inode.i_block[0]);
	}
	printf("\n");

	// read directories
	if (ftype == 'd') {
		for (int i = 0; i < 12; i++) {
			read_dinode(0, index+1, inode.i_block[i]);
		}
		read_dinode(1, index+1, inode.i_block[12]);
		read_dinode(2, index+1, inode.i_block[13]);
		read_dinode(3, index+1, inode.i_block[14]);
		return;
	}
	if (ftype == 'f' || ftype == 'd') {
		read_indir_block(1, index+1, 12, inode.i_block[12]);
		read_indir_block(2, index+1, 12+ptr_per_block, inode.i_block[13]);
		read_indir_block(3, index+1, 12+ptr_per_block+ptr_per_block*ptr_per_block,
			inode.i_block[14]);
	}
 
}

void read_indir_block(unsigned int level_below, 
	unsigned int inode_num, unsigned int offset, unsigned int nblock) {
	if (nblock == 0) 
		return;
	unsigned int block_ref;
	for (unsigned int i = 0; i < ptr_per_block; i++) {
		mypread(fd, &block_ref, sizeof(block_ref), get_offset(nblock)+i*sizeof(block_ref));
		if (block_ref == 0) continue;	
		printf("INDIRECT,%u,%u,%u,%u,%u\n", inode_num, level_below, 
			offset+i, nblock, block_ref);
		if (level_below > 1) {
			unsigned int new_offset;
			if (level_below == 2) new_offset = offset + i*ptr_per_block;
			else new_offset = offset + i * ptr_per_block * ptr_per_block;
			read_indir_block(level_below-1, inode_num, new_offset, block_ref);
		}
	}
}

void read_dinode(int level, unsigned int parent, unsigned int nblock) {
	if (nblock == 0) return;
	if (level == 0) {
		struct ext2_dir_entry dir;
		for (unsigned int offset = 0; offset < block_size; offset += dir.rec_len) {
			mypread(fd, &dir, sizeof(dir), get_offset(nblock)+offset);
			if (dir.inode == 0) continue;
			printf("DIRENT,%u,%u,%u,%u,%u,'%s'\n", parent, offset, dir.inode,
				dir.rec_len, dir.name_len, dir.name); 
		}	
		return;
	}
	unsigned int offset = get_offset(nblock);
	unsigned int nexlev_block;
	for (unsigned nexlev_offset = 0; nexlev_offset < block_size; 
		nexlev_offset += 4) {
		mypread(fd, &nexlev_block, sizeof(nexlev_block), offset+nexlev_offset);
		read_dinode(level-1, parent, nexlev_block);
	}

}

unsigned int get_offset(unsigned int nblock) {
	if (block_size == 1024) 
		return nblock * 1024;
	return nblock * block_size;
}

void get_time(const time_t time, char* str) {
	struct tm *gmt;
	gmt = gmtime(&time);
	strftime(str, 50, "%m/%d/%y %H:%M:%S", gmt);
}

ssize_t mypread(int fd, void *buf, size_t count, off_t offset) {
	ssize_t ret = pread(fd, buf, count, offset);
	if (ret == -1) {
		fprintf(stderr, "ERROR: 'pread()' - %s\n", strerror(errno));
		exit(1);
	}
	return ret;
}
