#!/usr/bin/python

#NAME: Justin Ma

import sys, string, locale, csv

# there should only be one superBlock (length should not exceed 1)
superBlock = []
freeBlocks = []
freeInodes = []
freeNodes = []
inodes = []
inodeNs = []
dirents = []
indirects = []
links = []
bases = []
blocks = {}

causedError = 0

class SuperBlock:
    def __init__(self, line):
        self.blockCount = int(line[1])
        self.inodeCount = int(line[2])
        self.blockSize = int(line[3])
        self.inodeSize = int(line[4])
        self.blocksPerGroup = int(line[5])
        self.inodesPerGroup = int(line[6])
        self.firstInode = int(line[7])
        
class Inode:
    def __init__(self, line):
        self.inode = int(line[1])
        self.type = line[2]
        self.mode = int(line[3])
        self.uid = int(line[4])
        self.gid = int(line[5])
        self.linkCount = int(line[6])
        self.ctime = line[7]
        self.mtime = line[8]
        self.atime = line[9]
        self.size = int(line[10])
        self.blocks = int(line[11])
        self.block = []
        self.block.append(int(line[12]))
        if not line[2] == 's':
            self.block.append(int(line[13]))
            self.block.append(int(line[14]))
            self.block.append(int(line[15]))
            self.block.append(int(line[16]))
            self.block.append(int(line[17]))
            self.block.append(int(line[18]))
            self.block.append(int(line[19]))
            self.block.append(int(line[20]))
            self.block.append(int(line[21]))
            self.block.append(int(line[22]))
            self.block.append(int(line[23]))
            self.firstInd = int(line[24])
            self.secondInd = int(line[25])
            self.thirdInd = int(line[26])
        
class Dirent:
    def __init__(self, line):
        self.pInode = int(line[1])
        self.offset = int(line[2])
        self.inode = int(line[3])
        self.recLength = int(line[4])
        self.nameLength = int(line[5])
        self.name = line[6].rstrip()
        
class Indirect:
    def __init__(self, line):
        self.inode = int(line[1])
        self.level = int(line[2])
        self.offset = int(line[3])
        self.indirectCounter = int(line[4])
        self.block = int(line[5])

  


def directoryFunction():
    links = [0] * superBlock[0].inodeCount
    bases = [0] * superBlock[0].inodeCount
    bases[2] = 2

    for dir in dirents:
        if dir.inode <= superBlock[0].inodeCount and dir.inode not in freeNodes:
            if dir.name != "'..'" and dir.name != "'.'":
                bases[dir.inode] = dir.pInode
            
        if dir.inode > superBlock[0].inodeCount:
            print("DIRECTORY INODE {} NAME {} INVALID INODE {}".format(dir.pInode, dir.name, dir.inode))
            causedError = 1
	elif dir.inode in freeInodes:
            print("DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}".format(dir.pInode, dir.name, dir.inode))
       	    causedError = 1
	else:
            links[dir.inode] += 1
    
    for node in inodes:
        if node.linkCount != links[node.inode]:
            print("INODE {} HAS {} LINKS BUT LINKCOUNT IS {}".format(node.inode, links[node.inode], node.linkCount))
	    causedError = 1
    for dir in dirents:
        if dir.name == "'.'" and dir.inode != dir.pInode:
            print("DIRECTORY INODE {} NAME '.' LINK TO INODE {} SHOULD BE {}".format(dir.pInode, dir.inode, dir.pInode))
            causedError = 1
	elif dir.name == "'..'" and  dir.inode != bases[dir.pInode]:
		print("DIRECTORY INODE {} NAME '..' LINK TO INODE {} SHOULD BE {}".format(dir.pInode, dir.inode, bases[dir.pInode]))
 	    	causedError = 1



def indirectFunction():
    for block in indirects:
        indirectType = ""
        lev = block.level
        if lev == 1:
            indirectType = "INDIRECT"
        elif lev == 2:
            indirectType = "DOUBLE INDIRECT"
        elif lev == 3:
            indirectType = "TRIPLE INDIRECT"
            
        ref = block.block
        if ref != 0:
            if ref > superBlock[0].blockCount-1:
                print("INVALID {} BLOCK {} IN INODE {} AT OFFSET {}".format(indirectType, ref, block.inode, block.offset))
            	causedError = 1
	    elif ref < 8:
                print("RESERVED {} BLOCK {} IN INODE {} AT OFFSET {}".format(indirectType, ref, block.inode, block.offset))
                causedError = 1
	    else:
                blocks[ref] = [(block.inode, block.offset, indirectType)]
                
    for block in xrange(8, superBlock[0].blockCount):
        if block not in freeBlocks and block not in blocks:
            print("UNREFERENCED BLOCK {}".format(block))
	    causedError = 1
        elif block in blocks and len(blocks[block]) > 1:
            for par, j, indirectType in blocks[block]:
                print("DUPLICATE {}BLOCK {} IN INODE {} AT OFFSET {}".format(indirectType, block, par, j))
                causedError = 1    
    
    freeNodes = freeInodes

    for node in inodes:
        inodeNs.append(node.inode)
        if node.type != '0':
            if node.inode in freeInodes:
                print("ALLOCATED INODE {} ON FREELIST".format(node.inode))
                freeNodes.remove(node.inode)
	        causedError = 1
        elif node.inode not in freeInodes:
            print("UNALLOCATED NODE {} NOT ON FREELIST".format(node.inode))
            freeNodes.append(node.inode)
    	    causedError = 1
    for node in xrange(superBlock[0].firstInode, superBlock[0].inodeCount):
        if node not in freeInodes and node not in inodeNs:
            print("UNALLOCATED INODE {} NOT ON FREELIST".format(node))
            freeNodes.append(node)
            causedError = 1
        

        
       
def inodeFunction():
    for node in inodes:
        off = 0
        for block in node.block:
            if block != 0 and not node.type == 's':
                if block > superBlock[0].blockCount - 1:
                    print("INVALID BLOCK {} IN INODE {} AT OFFSET {}".format(block, node.inode, off))
                    causedError = 1
		elif block < 8:
                    print("RESERVED BLOCK {} IN INODE {} AT OFFSET {}".format(block, node.inode, off))
                    causedError = 1
		elif block in freeBlocks:
                    print("ALLOCATED BLOCK {} ON FREELIST".format(block))
                    causedError = 1
		elif block in blocks:
                    blocks[block].append((node.inode, off, ""))
                else:
                    blocks[block] = [(node.inode, off, "")]
            off += 1;
        if not node.type == 's':
            first = node.firstInd
            if first != 0:
                if first > superBlock[0].blockCount-1:
                    print("INVALID INDIRECT BLOCK {} IN INODE {} AT OFFSET 12".format(first, node.inode))
                    causedError = 1
		elif first < 8:
                    print("RESERVED INDIRECT BLOCK {} IN INODE {} AT OFFSET 12".format(first, node.inode))
                    causedError = 1
		elif first in freeBlocks:
                    print("ALLOCATED BLOCK {} ON FREELIST".format(first))
                    causedError = 1
		elif first in blocks:
                    blocks[first].append((node.inode, 12, "INDIRECT "))
                else:
                    blocks[first] = [(node.inode, 12, "INDIRECT ")]
            
            second = node.secondInd
            if second != 0:
                if second > superBlock[0].blockCount-1:
                    print("INVALID DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 268".format(second, node.inode))
                    causedError = 1
		elif second < 8:
                    print("RESERVED DOUBLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 268".format(second, node.inode))
                    causedError = 1
		elif second in freeBlocks:
                    print("ALLOCATED BLOCK {} ON FREELIST".format(second))
                    causedError = 1
		elif second in blocks:
                    blocks[second].append((node.inode, 268, "DOUBLE INDIRECT "))
                else:
                    blocks[second] = [(node.inode, 268, "DOUBLE INDIRECT ")]
            
            third = node.thirdInd
            if third != 0:
                if third > superBlock[0].blockCount-1:
                    print("INVALID TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 65804".format(third, node.inode))
                    causedError = 1
		elif third < 8:
                    print("RESERVED TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 65804".format(third,node.inode))
                    causedError = 1
		elif third in freeBlocks:
                    print("RESERVED TRIPLE INDIRECT BLOCK {} IN INODE {} AT OFFSET 65804".format(third, node.inode))
                    causedError = 1
		elif third in blocks:
                    blocks[third].append((node.inode, 65804, "TRIPLE INDIRECT "))
                else:
                    blocks[third] = [(node.inode, 65804, "TRIPLE INDIRECT ")]

       
        
def blockFunction(summary):
    for line in summary:
        components = line.split(',')
        if components[0] == 'SUPERBLOCK':
            superBlock.append(SuperBlock(components))
        elif components[0] == 'BFREE':
            freeBlocks.append(int(components[1]))
        elif components[0] == 'IFREE':
            freeInodes.append(int(components[1]))
        elif components[0] == 'INODE':
            inodes.append(Inode(components))
        elif components[0] == 'DIRENT':
            dirents.append(Dirent(components))
        elif components[0] == 'INDIRECT':
            indirects.append(Indirect(components))
        
        
        
if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.stderr.write('This program requires the format \'./lab3b summaryName\'\n')
        exit(1)
        
    summary = open(sys.argv[1], 'r')
    if not summary:
        sys.stderr.write('The file could not be opened.\n')
        exit(1)
    
    blockFunction(summary)
    inodeFunction()
    indirectFunction()
    directoryFunction()
    summary.close()
    
    if causedError == 1:
	exit(2)
    else:
	exit(0)
