#!/usr/bin/env python3

import argparse, csv, collections, sys

reserved = [0] # reserved for root block
inodes = []
total_blks = None
fir_data_blk = None
fir_inode_blk = None
fir_nonres_inode_blk = None # first non-reserved iblock
bfree_list = [] # list of block numbers of free blocks
ifree_list = [] # list of inode numbers of free inodes
used_inodes = [] # list of block numbers of used inode
inode_size = None
blk_size = None
total_inodes = None

# key: parent inode number; value: list of (child_inode_number, file_name)
dirs = collections.defaultdict(list) 

class Inode:
    def __init__(self, is_free, row, is_indirect):
        self.is_free = is_free
        self.is_indirect = is_indirect
        self.inum = int(row[1])
        self.blocks = []
        self.offset = 0
        self.isdir = False
        self.blktag = None
        global ifree_list, used_inodes
        if is_free: 
            ifree_list.append(self.inum)
            if self.inum in used_inodes:
                print(f"ALLOCATED INODE {self.inum} ON FREELIST")
            return

        self.mode = int(row[3])
        self.isdir = row[2] == 'd'
        if self.mode == 0:
            print(f"UNALLOCATED INODE {self.inum} NOT ON FREELIST")
        else: used_inodes.append(self.inum)
        if not is_indirect: # if INODE
            if self.inum in ifree_list:
                print(f"ALLOCATED INODE {self.inum} ON FREELIST")
            self.links_count = int(row[6])
            if len(row) == 27:
                for block in row[12:]:
                    self.blocks.append(int(block))
            self.blktag = "BLOCK"
        else: # if INDIRECT
            self.blocks = [int(row[5])]
            self.offset =  int(row[3])
            if row[2] == '1': self.blktag = "INDIRECT BLOCK"
            elif row[2] == '2': self.blktag = "DOUBLE INDIRECT BLOCK"
            elif row[2] == '3': self.blktag = "TRIPLE INDIRECT BLOCK"

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'csv',action="store", nargs=1, type=str,
        help="read lines from the file in_file\n")
    try:
        args = parser.parse_args()
    except:
        print("ERROR: wrong number of arguments", file=sys.stderr)
        sys.exit(1)
    try:
        with open(args.csv[0]) as file:
            rows = csv.reader(file)
            for row in rows:
                if row[0] == 'BFREE': bfree_list.append(int(row[1]))
                elif row[0] == 'INODE': inodes.append(Inode(False, row, is_indirect=False))
                elif row[0] == 'IFREE': inodes.append(Inode(True, row, is_indirect=False))
                elif row[0] == 'INDIRECT': inodes.append(Inode(False, row, is_indirect=True))
                elif row[0] == 'DIRENT': proc_dir(row)
                elif row[0] == 'GROUP': read_descriptor(row)
                elif row[0] == 'SUPERBLOCK': read_superblock(row)

                else: pass
    except: 
        print("ERROR: bogus file name", file=sys.stderr)
        sys.exit(1)
    check_blocks()
    check_inodes()
    check_dirs()

def proc_dir(row):
    global dirs
    dirs[int(row[1])].append((int(row[3]), row[6]))

def check_dirs():
    global inodes, dirs, total_inodes, used_inodes
    inode_ref_array = collections.defaultdict(int)
    inode_par_array = {}
    for inode in inodes:
        if not inode.isdir: continue
        par_ino = inode.inum
        for child_ino, child_name in dirs[par_ino]:
            if child_ino < 0 or child_ino >= total_inodes:
                print(f"DIRECTORY INODE {par_ino} NAME {child_name} INVALID INODE {child_ino}")
                continue
            if child_ino not in used_inodes:
                print(f"DIRECTORY INODE {par_ino} NAME {child_name} UNALLOCATED INODE {child_ino}")
            if child_name == "'.'" and child_ino != par_ino:
                print(f"DIRECTORY INODE {par_ino} NAME '.' LINK TO INODE {child_ino} SHOULD BE {par_ino}")
            if child_name != "'.'" and child_name != "'..'":
                inode_par_array[child_ino] = par_ino
            inode_ref_array[child_ino] += 1
    for inode in inodes:
        if inode.is_free or inode.is_indirect: continue
        par_ino = inode.inum
        if inode_ref_array[par_ino] != inode.links_count:
            print(f"INODE {par_ino} HAS {inode_ref_array[par_ino]} LINKS BUT LINKCOUNT IS {inode.links_count}")
        if not inode.isdir: continue
        for child_ino, child_name in dirs[par_ino]:
            if child_name == "'..'" and par_ino not in inode_par_array and child_ino != par_ino:
                print(f"DIRECTORY INODE {par_ino} NAME '..' LINK TO INODE {child_ino} SHOULD BE {par_ino}")
            if child_name == "'..'" and par_ino in inode_par_array and child_ino != inode_par_array[par_ino]:
                print(f"DIRECTORY INODE {par_ino} NAME '..' LINK TO INODE {child_ino} SHOULD BE {inode_par_array[par_ino]}")

def check_inodes():
    global fir_nonres_inode_blk, fir_data_blk, ifree_list, used_inodes
    for i in range(fir_nonres_inode_blk, total_inodes):
        if i in ifree_list and i in used_inodes:
            print(f"ALLOCATED INODE {i} ON FREELIST")
        if i not in ifree_list and i not in used_inodes:
            print(f"UNALLOCATED INODE {i} NOT ON FREELIST")

def check_blocks():
    global inodes, reserved, total_blks, fir_data_blk, bfree_list
    block_map = [False]*(total_blks - fir_data_blk)
    for inode in inodes:
        blktag, inum, offset = inode.blktag, inode.inum, inode.offset
        for i, block in enumerate(inode.blocks):
            ptr_per_block = int(blk_size / 4)
            if i == 12:
                blktag = "INDIRECT BLOCK"
                offset = 12
            if i == 13:
                blktag = "DOUBLE INDIRECT BLOCK"
                offset = 12 + ptr_per_block
            if i == 14:
                blktag = "TRIPLE INDIRECT BLOCK"
                offset = 12 + ptr_per_block + ptr_per_block*ptr_per_block

            if block < 0 or block >= total_blks:
                print(f"INVALID {blktag} {block} IN INODE {inum} AT OFFSET {offset}")
            if block > 0 and block < fir_data_blk: 
                print(f"RESERVED {blktag} {block} IN INODE {inum} AT OFFSET {offset}")
            if block in bfree_list:
                print(f"ALLOCATED BLOCK {block} ON FREELIST")
            if block >= fir_data_blk and block < total_blks:
                if block_map[block - fir_data_blk]:
                    print(f"DUPLICATE {blktag} {block} IN INODE {inum} AT OFFSET {offset}")
                    if block_map[block - fir_data_blk] != True: # if the first occurence hasn't been reported
                        fir_inode = block_map[block - fir_data_blk]
                        print(f"DUPLICATE {fir_inode.blktag} {block} IN INODE {fir_inode.inum} AT OFFSET {fir_inode.offset}")
                        block_map[block - fir_data_blk] = True
                else: 
                    block_map[block - fir_data_blk] = inode
    for bnum, is_used in enumerate(block_map, start=fir_data_blk):
        if not is_used and bnum not in bfree_list: # not mentioned in free list or inode
            print(f"UNREFERENCED BLOCK {bnum}")

def read_descriptor(descriptor):
    global reserved, fir_data_blk, fir_inode_blk, inode_size, blk_size, total_inodes
    reserved.append(int(descriptor[6])) # reserved for block bitmap
    reserved.append(int(descriptor[7])) # reserved for i-node bitmap
    fir_inode_blk = int(descriptor[8])
    total_inodes = int(descriptor[3])
    fir_data_blk = int(fir_inode_blk + (int(descriptor[3])*inode_size)/blk_size)  # first-inode + total inodes

def read_superblock(sb):
    global total_blks, reserved, inode_size, blk_size, fir_nonres_inode_blk
    reserved.append(1) # reserved for super block
    if sb[3] == '1024': reserved.append(2) # reserved for group descriptor
    total_blks = int(sb[1])
    inode_size = int(sb[4])
    blk_size = int(sb[3])
    fir_nonres_inode_blk = int(sb[7])

if __name__ == "__main__":
    main()

