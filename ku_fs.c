#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct inode{
    unsigned int fsize;			//	file 이 몇 byte 인지 한 block은 4kb인데 4kb의 배수가 아닐경우 넉넉하게 할당
    unsigned int blocks;		//	block 개수
    unsigned int pointer[12];	//	data block의 번호를 가지고 있는 배열
} iNode;

typedef struct entity{
    char inum;
    char fName[3];
} Entity;

unsigned int blockSize = 4096;			//	4KB
unsigned int blockNum = 64;
unsigned int iNodeSize = 256;			//	256B
unsigned int entitySize = 4;			//	4byte

void* disk;
void* iBmap;		//	i-bmap 시작주소
void* dBmap;		//	d-bmap 시작주소
void* iBlock;		//	inode block 시작주소
void* dBlock;		//	data block 시작주소

void fsInit();
void write(char* fileName, int size);
void read(char* fileName, int size);
void delete(char* fileName);
void makeRoot();
char getEntityIdx();
char getINodeIdx();
char getDataBlockIdx();
void print();
int isExist(char* fName);

int main(int argc, char* argv[]) {
    FILE *fd = NULL;
    disk = calloc(blockNum, blockSize);			//	265KB의 disk 할당
    fsInit();					//	file system initialize
    char fileName[3];			//	텍스트 파일에서의 file name
    char oper;					//	텍스트 파일에서의 operation
    int size;					//	텍스트 파일에서의 file size
    // print();
    fd = fopen(argv[1], "r");
    while(fscanf(fd, "%s %c %d", fileName, &oper, &size) != -1){
        if(oper == 'w'){
            write(fileName, size);
        }
        else if(oper == 'r')
            read(fileName, size);
        else
            delete(fileName);
    }
    fclose(fd);
    print();

    return 0;
}

void fsInit(){
    iBmap = disk + blockSize;
    dBmap = disk + (blockSize * 2);
    iBlock = disk + (blockSize * 3);
    dBlock = disk + (blockSize * 8);
    *(char*)iBmap = 1;			//	reserved : not use
    *(char*)(iBmap + 1) = 1;	//	bad block
    makeRoot();			//	root directory 초기화
}

void makeRoot(){
    Entity* root = (Entity*)calloc(80, sizeof(Entity));		//	4byte짜리 80개 할당
    dBlock = root;
    iNode* n = (iNode*)malloc(sizeof(iNode));
    n->fsize = 320;			//	root directory의 file size : 4*80
    n->blocks = 1;			//	root directory의 block의 개수 : 1개
    n->pointer[0] = 0;		//	root directory가 쓰는 block의 위치 : 0번 index
    *(iNode*)(iBlock + iNodeSize * 2) = *n;
    *(char*)(iBmap + 2) = 1;	//	root directory inode bmap
    *(char*)dBmap = 1;			//	root directory data block bmap
}

void write(char* fileName, int size){
    if(isExist(fileName) != -1){
        printf("Already exists\n");
        return;
    }
    char entityIdx = getEntityIdx();
    char inodeIdx = getINodeIdx();
    if(entityIdx == -1){
        printf("No Space\n");
        return;
    }
    Entity *e = (Entity*)malloc(sizeof(Entity));
    e->inum = inodeIdx;
    e->fName[0] = fileName[0];
    e->fName[1] = fileName[1];
    *(Entity*)(dBlock + entityIdx) = *e;
    iNode *in = (iNode*)malloc(sizeof(iNode));
    in->fsize = size;
    in->blocks = size / blockSize;
    if(size % blockSize)
        in->blocks++;
    int i;
    int arr[in->blocks];
    for(i = 0; i < in->blocks; i++){
        char temp = getDataBlockIdx();
        if(temp == -1){
            printf("No space!\n");
            break;
        }
        arr[i] = (int)temp;
    }
    for(i = 0; i < in->blocks;i++){
        in->pointer[i] = arr[i];
    }
    *(iNode*)(iBlock + inodeIdx * iNodeSize) = *in;
    for(i = 0; i < size; i++){			//	datablock에 file name의 앞글자를 size 개 만큼 입력
        *(char*)(dBlock + in->pointer[0] * blockSize + i) = e->fName[0];
    }
}

void read(char* fileName, int size){
    int entityIdx = isExist(fileName);
    if(entityIdx == -1) {
        printf("No such file\n");
        return;
    }
    char iNum = ((Entity*)(dBlock + entityIdx * entitySize))->inum;
    iNode* in = (iNode*)(iBlock + iNum * iNodeSize);
    int fileSize = in->fsize;
    int i;
    if(fileSize < size){
        for(i = 0; i < fileSize; i++){
            printf("%c", *(char*)(dBlock + in->pointer[0] * blockSize + i));
        }
        printf("\n");
    }else{
        for(i = 0; i < size; i++)
            printf("%c", *(char*)(dBlock + in->pointer[0] * blockSize + i));
        printf("\n");
    }
}

void delete(char* fileName){
    int entityIdx = isExist(fileName);
    if(entityIdx == -1)
        printf("No such file\n");
    ((Entity*)(dBlock + entityIdx * 4))->inum = 0;
}

char getEntityIdx(){
    int i = 0;
    for(i = 0; i < 80; i+=entitySize){
        if((*(char*)(dBlock + i)) == 0){
            return (char)i;
        }
    }
    return -1;
}

char getINodeIdx(){
    int i;
    for(i = 0; i < 56; i++){
        if(*(char*)(iBmap + i) == 0){
            *(char*)(iBmap + i) = 1;
            return (char)i;
        }
    }
    return -1;
}

char getDataBlockIdx(){
    int i;
    for(i = 1; i < 56; i++){
        if((*(char*)(dBmap + i)) == 0){
            (*(char*)(dBmap + i)) = 1;
            return (char)i;
        }
    }
    return -1;
}

int isExist(char* fileName){
    int i;
    for(i = 0; i < 80; i+=entitySize){
        if(strcmp(((Entity*)(dBlock + i))->fName, fileName) == 0)
            return i/4;		//	있는 번호
        if(((Entity*)(dBlock + i))->inum == 0)
            continue;
    }
    return -1;		//	없음
}

void print(){
    FILE *fp;
    fp = fopen("output.txt", "w");
    for(int i = 0; i < 4096 * 64; i++){ // 4kb (4096 bytes) * 64 blocks
        fprintf(fp, "%.2x ", *((unsigned char *)(disk + i)));
    }
}