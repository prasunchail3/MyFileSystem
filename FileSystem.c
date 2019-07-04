#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define STRING_SIZE 10

//--------------MISCELLANEOUS------------------//
#define LSH_RL_BUFSIZE 1024
int min(int a, int b) {
	if(a < b)
		return a;
	return b;
}

char *lsh_read_line(void) {
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } 
    else if (c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } 
    else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
char **lsh_split_line(char *line) {
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "lsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
//---------------------------------------------//

//---------------DISK TEMPLATE----------------//
struct Disk {
	unsigned char* buffer;
	int totalSize; //in MB
	int blockSize; //in B
};

void createDisk(struct Disk* disk, int totalSize, int blockSize);
void readBlock(struct Disk* disk, int blockNumber, unsigned char* data);
void writeBlock(struct Disk* disk, int blockNumber, unsigned char* data);
//--------------------------------------------//

//---------------DISK CODE--------------------//
void createDisk(struct Disk* disk, int totalSize, int blockSize) {
	disk -> totalSize = totalSize;
	disk -> blockSize = blockSize;
	disk -> buffer = (unsigned char*)malloc(totalSize * (int)(pow(2, 20)) * sizeof(unsigned char));
}

void readBlock(struct Disk* disk, int blockNumber, unsigned char* data) {
	data = realloc(data, disk -> blockSize);
	memcpy(data, (disk -> buffer) + blockNumber * (disk -> blockSize), disk -> blockSize);
}

void writeBlock(struct Disk* disk, int blockNumber, unsigned char* data) {
	data = realloc(data, disk -> blockSize);
	memcpy((disk -> buffer) + blockNumber * (disk -> blockSize), data, disk -> blockSize);
}
//--------------------------------------------//

//-------------FILE SYSTEM TEMPLATE-----------//
#define POINTERS_PER_INODE 5 // Must be >= 5
#define VALID_MAGIC_NUMBER 1234

struct SuperBlock {
	int magicNumber;
	int totalBlockCount;
	int inodeCount, mountCount, directoryCount, dataCount;
	int inodeBitmap, mountBitmap, directoryBitmap, dataBitmap;
	int inode, mount, directory, data;
};

struct Inode {
	int isDirectory;
	int sizeofFile;
	int blockNumbers[POINTERS_PER_INODE];
};

struct Mount {
	unsigned char diskName[STRING_SIZE];
	struct Disk* location;
	int magicNumber;
};

struct Directory {
	unsigned char fileName[STRING_SIZE];
	int inumber;
};

struct Path {
	struct Disk* location;
	int inumber;
};

struct PseudoPath {
	struct Disk* location;
	char fileName[10];
};

void fillWithZero(unsigned char* buffer, int tot);

void createFileSystem(struct Disk* disk);
void getSuperBlock(struct Disk* disk, struct SuperBlock* sb);
void getInode(struct Disk* disk, int base, int inumber, struct Inode* i1);
void setInode(struct Disk* disk, int base, int inumber, struct Inode* i1);
void getMount(struct Disk* disk, int base, int inumber, struct Mount* i1);
void setMount(struct Disk* disk, int base, int inumber, struct Mount* i1);
void getDirectory(struct Disk* disk, int base, int inumber, struct Directory* i1);
void setDirectory(struct Disk* disk, int base, int inumber, struct Directory* i1);

void pathResolution(unsigned char* buffer, struct Path* path, struct Disk* rootDisk);
void partition(unsigned char* buffer, struct PseudoPath* path, struct Disk* rootDisk);

int createFile(struct Disk* disk, unsigned char name[10]);
void releaseFile(struct Disk* disk, int inumber);
void removeFile(struct Disk* disk, int inumber);
int writeFile(struct Disk* disk, int inumber, unsigned char* buffer, int len);
void readFile(struct Disk* disk, int inumber, unsigned char* buffer);

int mountFileSystem(struct Disk* diskBase, struct Disk* diskMount, char name[10]);
void unmountFileSystem(struct Disk* diskBase, struct Disk* diskMount, char name[10]);

void ls(struct Disk* disk);
//--------------------------------------------//
//-------------FILE SYSTEM CODE---------------//
void fillWithZero(unsigned char* buffer, int tot) {
	int i;
	for(i = 0; i < tot; i += 1)
		buffer[i] = 0;
}

void createFileSystem(struct Disk* disk) {
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	sb -> magicNumber = VALID_MAGIC_NUMBER;
	sb -> totalBlockCount = (disk -> totalSize * (int)pow(2, 20)) / disk -> blockSize;

	int blockCountSuperBlock = 1;
	int blockCountInodeBitmap = 1;
	int blockCountMountBitmap = 1;
	int blockCountDirectoryBitmap = 1;
	int blockCountDataBitmap = 1;
	int blockCountInode = (int)(0.1 * sb -> totalBlockCount);
	int blockCountMount = (int)(0.05 * sb -> totalBlockCount);
	int blockCountDirectory = (int)(0.1 * sb -> totalBlockCount);

	sb -> inodeBitmap = blockCountSuperBlock;
	sb -> mountBitmap = sb -> inodeBitmap + blockCountInodeBitmap;
	sb -> directoryBitmap = sb -> mountBitmap + blockCountMountBitmap;
	sb -> dataBitmap = sb -> directoryBitmap + blockCountDirectoryBitmap;
	sb -> inode = sb -> dataBitmap + blockCountDataBitmap;
	sb -> mount = sb -> inode + blockCountInode;
	sb -> directory = sb -> mount + blockCountMount;
	sb -> data = sb -> directory + blockCountDirectory;	

	int blockCountData = sb -> totalBlockCount - sb -> data;

	int inodePerBlock = disk -> blockSize / sizeof(struct Inode);
	int mountPerBlock = disk -> blockSize / sizeof(struct Mount);
	int directoryPerBlock = disk -> blockSize / sizeof(struct Directory);
	int dataPerBlock = 1;

	sb -> inodeCount = min(blockCountInode * inodePerBlock, blockCountInodeBitmap * disk -> blockSize);
	sb -> mountCount = min(blockCountMount * mountPerBlock, blockCountMountBitmap *  disk -> blockSize);
	sb -> directoryCount = min(blockCountDirectory * directoryPerBlock, blockCountDirectoryBitmap * disk -> blockSize);
	sb -> dataCount = min(blockCountData * dataPerBlock, blockCountDataBitmap * disk -> blockSize);

	sb -> inodeCount = min(sb -> inodeCount, sb -> directoryCount);
	sb -> directoryCount = sb -> inodeCount;

	writeBlock(disk, 0, (unsigned char*)sb);

	fillWithZero(temp, disk -> blockSize);
	writeBlock(disk, sb -> inodeBitmap, temp);
	writeBlock(disk, sb -> mountBitmap, temp);
	writeBlock(disk, sb -> directoryBitmap, temp);
	writeBlock(disk, sb -> dataBitmap, temp);
}

void getSuperBlock(struct Disk* disk, struct SuperBlock* sb) {
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, 0, temp);
	memcpy(sb, (struct SuperBlock*)temp, sizeof(struct SuperBlock));
}

void getInode(struct Disk* disk, int base, int inumber, struct Inode* i1) {
	int inodePerBlock = disk -> blockSize / sizeof(struct Inode);
	int blockNo = base + inumber / inodePerBlock;
	int offsetNo = inumber % inodePerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(i1, (struct Inode*)(temp + offsetNo * sizeof(struct Inode)), sizeof(struct Inode));
}

void setInode(struct Disk* disk, int base, int inumber, struct Inode* i1) {
	int inodePerBlock = disk -> blockSize / sizeof(struct Inode);
	int blockNo = base + inumber / inodePerBlock;
	int offsetNo = inumber % inodePerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(temp + offsetNo * sizeof(struct Inode), (unsigned char*)i1, sizeof(struct Inode));
	writeBlock(disk, blockNo, temp);
}

void getMount(struct Disk* disk, int base, int inumber, struct Mount* i1) {
	int mountPerBlock = disk -> blockSize / sizeof(struct Mount);
	int blockNo = base + inumber / mountPerBlock;
	int offsetNo = inumber % mountPerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(i1, (struct Mount*)(temp + offsetNo * sizeof(struct Mount)), sizeof(struct Mount));
}

void setMount(struct Disk* disk, int base, int inumber, struct Mount* i1) {
	int mountPerBlock = disk -> blockSize / sizeof(struct Mount);
	int blockNo = base + inumber / mountPerBlock;
	int offsetNo = inumber % mountPerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(temp + offsetNo * sizeof(struct Mount), (unsigned char*)i1, sizeof(struct Mount));
	writeBlock(disk, blockNo, temp);
}

void getDirectory(struct Disk* disk, int base, int inumber, struct Directory* i1) {
	int directoryPerBlock = disk -> blockSize / sizeof(struct Directory);
	int blockNo = base + inumber / directoryPerBlock;
	int offsetNo = inumber % directoryPerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(i1, (struct Directory*)(temp + offsetNo * sizeof(struct Directory)), sizeof(struct Directory));
}

void setDirectory(struct Disk* disk, int base, int inumber, struct Directory* i1) {
	int directoryPerBlock = disk -> blockSize / sizeof(struct Directory);
	int blockNo = base + inumber / directoryPerBlock;
	int offsetNo = inumber % directoryPerBlock;
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));
	readBlock(disk, blockNo, temp);
	memcpy(temp + offsetNo * sizeof(struct Directory), (unsigned char*)i1, sizeof(struct Directory));
	writeBlock(disk, blockNo, temp);
}

void pathResolution(unsigned char* buffer1, struct Path* path, struct Disk* rootDisk) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Mount* m1 = (struct Mount*)malloc(sizeof(struct Mount));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* buffer = (unsigned char*)malloc(sizeof(buffer1));
	strcpy(buffer, buffer1);

	int i, j, blockNo, offsetNo, flag;
	for(i = 0; i < strlen(buffer); i += 1) {
		if(buffer[i] == '\\')
			buffer[i] = ' ';
	}
	char** args = lsh_split_line(buffer);
	struct Disk* currDisk = rootDisk;
	int currInumber = -1;

	i = 0;
	while(args[i] != NULL) {
		getSuperBlock(currDisk, sb);
		unsigned char* temp = (unsigned char*)malloc(currDisk -> blockSize);

		if(currInumber == -1) {
			flag = 0;
			readBlock(currDisk, sb -> mountBitmap, temp);
			for(j = 0; j < sb -> mountCount; j += 1) {
				if(temp[j] == 0)
					continue;
				getMount(currDisk, sb -> mount, j, m1);
				if(m1 -> magicNumber == VALID_MAGIC_NUMBER && strcmp(m1 -> diskName, args[i]) == 0) {
					flag = 1;
					currDisk = m1 -> location;
					currInumber = -1;
					break;
				}
			}
			if(flag == 1) {
				i += 1;
				continue;
			}
			readBlock(currDisk, sb -> directoryBitmap, temp);
			for(j = 0; j < sb -> directoryCount; j += 1) {
				if(temp[j] == 0)
					continue;
				getDirectory(currDisk, sb -> directory, j, d1);
				if(strcmp(d1 -> fileName, args[i]) == 0) {
					flag = 1;
					currInumber = d1 -> inumber;
					break;
				}
			}
			if(flag == 0) {
				printf("Path is invalid\n");
				path -> location = NULL;
				path -> inumber = -1;
				return;
			}
		}
		else 
			break;

		i += 1;
	}
	path -> location = currDisk;
	path -> inumber = currInumber;
}

void partition(unsigned char* buffer1, struct PseudoPath* path, struct Disk* rootDisk) {
	unsigned char* buffer = (unsigned char*)malloc(sizeof(buffer1));
	strcpy(buffer, buffer1);
	int i;
	struct Path* p1 = (struct Path*)malloc(sizeof(struct Path));
	for(i = strlen(buffer) - 1; i >= 0; i -= 1) {
		if(buffer[i] == '\\') {
			buffer[i] = 0;
			pathResolution(buffer, p1, rootDisk);
			path -> location = p1 -> location;
			strcpy(path -> fileName, buffer + i + 1);
			return;
		}
	}
	path -> location = rootDisk;
	strcpy(path -> fileName, buffer);
}

int createFile(struct Disk* disk, unsigned char name[10]) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	readBlock(disk, sb -> directoryBitmap, temp);
	int i;
	for(i = 0; i < sb -> directoryCount; i += 1) {
		if(temp[i] == 0)
			continue;
		getDirectory(disk, sb -> directory, i, d1);
		if(strcmp(d1 -> fileName, name) == 0)
			return -2; //Duplicate name
	}
	for(i = 0; i < sb -> inodeCount; i += 1) {
		if(temp[i] == 1)
			continue;
		temp[i] = 1;
		writeBlock(disk, sb -> inodeBitmap, temp);
		writeBlock(disk, sb -> directoryBitmap, temp);
		i1 -> isDirectory = 0;
		i1 -> sizeofFile = 0;
		setInode(disk, sb -> inode, i, i1);
		strcpy(d1 -> fileName, name);
		d1 -> inumber = i;
		setDirectory(disk, sb -> directory, i, d1);
		return 1;
	}
	return -1; //Space not available
}

void releaseFile(struct Disk* disk, int inumber) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	getInode(disk, sb -> inode, inumber, i1);
	i1 -> sizeofFile = 0;
	setInode(disk, sb -> inode, inumber, i1);

	readBlock(disk, sb -> dataBitmap, temp);
	int i;
	for(i = 0; i < POINTERS_PER_INODE; i += 1) 
		temp[i1 -> blockNumbers[i]] = 0;
	writeBlock(disk, sb -> dataBitmap, temp);
}

void removeFile(struct Disk* disk, int inumber) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	releaseFile(disk, inumber);
	readBlock(disk, sb -> inodeBitmap, temp);
	temp[inumber] = 0;
	writeBlock(disk, sb -> inodeBitmap, temp);
	readBlock(disk, sb -> directoryBitmap, temp);
	temp[inumber] = 0;
	writeBlock(disk, sb -> directoryBitmap, temp);

}

int writeFile(struct Disk* disk, int inumber, unsigned char* buffer, int len) {
	releaseFile(disk, inumber);
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	if(len > disk -> blockSize * POINTERS_PER_INODE)
		return -1; //Size exceeded
	int i, blocksRequired = len / disk -> blockSize, j = 0;
	if(len % disk -> blockSize != 0)
		blocksRequired += 1;
	readBlock(disk, sb -> dataBitmap, temp);
	for(i = 0; i < sb -> dataCount; i += 1) {
		if(j == blocksRequired)
			break;
		if(temp[i] == 0)
			j += 1;
	}
	if(j != blocksRequired)
		return -2; // Blocks not available
	getInode(disk, sb -> inode, inumber, i1);
	i1 -> sizeofFile = len;
	j = 0;
	for(i = 0; i < sb -> dataCount; i += 1) {
		if(j == blocksRequired)
			break;
		if(temp[i] == 1)
			continue;
		temp[i] = 1; //write this
		i1 -> blockNumbers[j] = i;
		writeBlock(disk, sb -> data + i, buffer + j * disk -> blockSize);
		j += 1;
	}
	writeBlock(disk, sb -> dataBitmap, temp);	
	setInode(disk, sb -> inode, inumber, i1);
	return 1;
}

void readFile(struct Disk* disk, int inumber, unsigned char* buffer) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	getInode(disk, sb -> inode, inumber, i1);
//	printf("%d\n", i1 -> sizeofFile);
	int i, blocksRequired = i1 -> sizeofFile / disk -> blockSize, j;
	if(i1 -> sizeofFile % disk -> blockSize != 0)
		blocksRequired += 1;
	for(i = 0; i < blocksRequired; i += 1) {
		readBlock(disk, sb -> data + i1 -> blockNumbers[i], temp);
		j = min(disk -> blockSize, i1 -> sizeofFile - i * disk -> blockSize);
		strncpy(buffer + i * disk -> blockSize, temp, j);
	}
}

int mountFileSystem(struct Disk* disk, struct Disk* diskMount, char name[10]) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Mount* m1 = (struct Mount*)malloc(sizeof(struct Mount));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	readBlock(disk, sb -> mountBitmap, temp);
	int i;
	for(i = 0; i < sb -> mountCount; i += 1) {
		if(temp[i] == 0)
			continue;
		getMount(disk, sb -> mount, i, m1);
		if(m1 -> location == diskMount)
			return -2; //Duplicate exists
	}
	for(i = 0; i < sb -> mountCount; i += 1) {
		if(temp[i] == 1)
			continue;
		temp[i] = 1;
		writeBlock(disk, sb -> mountBitmap, temp);
		strcpy(m1 -> diskName, name);
		m1 -> location = diskMount;
		m1 -> magicNumber = VALID_MAGIC_NUMBER;
		setMount(disk, sb -> mount, i, m1);
		return 1;
	}
	return -1; // Space not available
}

int renameFileSystem(struct Disk* disk, struct Disk* diskMount, char name[10]) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Mount* m1 = (struct Mount*)malloc(sizeof(struct Mount));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	readBlock(disk, sb -> mountBitmap, temp);
	int i;
	for(i = 0; i < sb -> mountCount; i += 1) {
		if(temp[i] == 0)
			continue;
		getMount(disk, sb -> mount, i, m1);
		if(m1 -> location != diskMount && strcmp(m1 -> diskName, name) == 0)
			return -1; //Duplicate will occur
	}
	for(i = 0; i < sb -> mountCount; i += 1) {
		if(temp[i] == 0)
			continue;
		getMount(disk, sb -> mount, i, m1);
		if(m1 -> location == diskMount) {
			strcpy(m1 -> diskName, name);
			setMount(disk, sb -> mount, i, m1);
			return 1;
		}
	}
	return -2; //disk not found
}

void ls(struct Disk* disk) {
	struct SuperBlock* sb = (struct SuperBlock*)malloc(sizeof(struct SuperBlock));
	struct Inode* i1 = (struct Inode*)malloc(sizeof(struct Inode));
	struct Directory* d1 = (struct Directory*)malloc(sizeof(struct Directory));
	unsigned char* temp = (unsigned char*)malloc(disk -> blockSize * sizeof(unsigned char));

	getSuperBlock(disk, sb);
	readBlock(disk, sb -> directoryBitmap, temp);
	int i;
	for(i = 0; i < sb -> directoryCount; i += 1) {
		if(temp[i] == 0)
			continue;
		getDirectory(disk, sb -> directory, i, d1);
		getInode(disk, sb -> inode, i, i1);
		printf("%d %s %d\n", i, d1 -> fileName, i1 -> sizeofFile);
	}
}
//--------------------------------------------//

int main() {
	struct Disk* root = (struct Disk*)malloc(sizeof(struct Disk));
	createDisk(root, 100, 2048);
	createFileSystem(root);
	struct Path* p1 = (struct Path*)malloc(sizeof(struct Path));
	struct Path* p2 = (struct Path*)malloc(sizeof(struct Path));
	struct PseudoPath* p3 = (struct PseudoPath*)malloc(sizeof(struct PseudoPath));
	unsigned char* buffer;

	unsigned char* line;
	char** args;
	int i, j;

	while(1) {
		printf("myfs> ");
		line = lsh_read_line();
		args = lsh_split_line(line);
		//mkfs osfile1 512 10MB
		if(strcmp(args[0], "mkfs") == 0) {
			struct Disk* d1 = (struct Disk*)malloc(sizeof(struct Disk));
			createDisk(d1, atoi(args[3]), atoi(args[2]));
			createFileSystem(d1);
			mountFileSystem(root, d1, args[1]);
		}
		//use osfile1 as C:
		else if(strcmp(args[0], "use") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			renameFileSystem(root, p1 -> location, args[3]);
		}
		//cp osfile3 C:\tesfile1 
		else if(strcmp(args[0], "cp") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			partition(args[2], p3, root);
			if(p3 -> location == NULL) 
				continue;
			createFile(p3 -> location, p3 -> fileName);
			pathResolution(args[2], p2, root);
			buffer = (unsigned char*)malloc(POINTERS_PER_INODE * p1 -> location -> blockSize);
			readFile(p1 -> location, p1 -> inumber, buffer);
			writeFile(p2 -> location, p2 -> inumber, buffer, strlen(buffer));		
		}
		//ls C:
		else if(strcmp(args[0], "ls") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			ls(p1 -> location);
		}
		//rm C:\testfile1
		else if(strcmp(args[0], "rm") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			removeFile(p1 -> location, p1 -> inumber);
		}
		//mv D:\testfile2 D:\testfile2a
		else if(strcmp(args[0], "mv") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			partition(args[2], p3, root);
			if(p3 -> location == NULL) 
				continue;
			createFile(p3 -> location, p3 -> fileName);
			pathResolution(args[2], p2, root);
			if(p2 -> location == NULL && p2 -> inumber == -1) 
				continue;
			buffer = (unsigned char*)malloc(POINTERS_PER_INODE * p1 -> location -> blockSize);
			readFile(p1 -> location, p1 -> inumber, buffer);
			writeFile(p2 -> location, p2 -> inumber, buffer, strlen(buffer));
			removeFile(p1 -> location, p1 -> inumber);		
		}
		//create C: filename
		else if(strcmp(args[0], "create") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
			createFile(p1 -> location, args[2]);
		}
		//display C:\filename
		else if(strcmp(args[0], "display") == 0) {
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
//			printf("%d %s %d\n", p1 -> location, args[1], p1 -> inumber);
			buffer = (unsigned char*)malloc(POINTERS_PER_INODE * p1 -> location -> blockSize);
			readFile(p1 -> location, p1 -> inumber, buffer);
			printf("%s\n", buffer);
		}
		//write C:\filename
		else if(strcmp(args[0], "write") == 0) {
			partition(args[1], p3, root);
			if(p3 -> location == NULL) 
				continue;
			createFile(p3 -> location, p3 -> fileName);
			pathResolution(args[1], p1, root);
			if(p1 -> location == NULL && p1 -> inumber == -1) 
				continue;
//			printf("%d %s %d\n", p3 -> location, p3 -> fileName, p1 -> inumber);
			line = lsh_read_line();
			writeFile(p1 -> location, p1 -> inumber, line, strlen(line));
		}
		//exit
		else if(strcmp(args[0], "exit") == 0)
			break;
	}
	return 0;
}
