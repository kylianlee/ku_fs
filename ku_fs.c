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
void printibmap();
void printdbmap();
int isExist(char* fName);

int main(int argc, char* argv[]) {
    FILE *fd = NULL;
    disk = calloc(blockNum, blockSize);			//	265KB의 disk 할당
    fsInit();					//	file system initialize
    char fileName[3];			//	텍스트 파일에서의 file name
    char oper;					//	텍스트 파일에서의 operation
    int size;					//	텍스트 파일에서의 file size
    fd = fopen(argv[1], "r");
    while(fscanf(fd, "%s %c %d", fileName, &oper, &size) != -1){
        if(oper == 'w'){
            // printdbmap();
            write(fileName, size);
            // printdbmap();
        }
        else if(oper == 'r')
            read(fileName, size);
        else
            delete(fileName);
    }
    fclose(fd);
    // printf("%hhd\n", *(char*)dBlock);
    // printf("%hhd\n", *(char*)(dBlock + 1));
    // printf("%hhd\n", getEntityIdx());
    // printf("%hhd\n", getINodeIdx());
    // printf("%hhd\n", getDataBlockIdx());

    return 0;
}

void fsInit(){
    iBmap = disk + blockSize;
    dBmap = disk + (blockSize * 2);
    iBlock = disk + (blockSize * 3);
    dBlock = disk + (blockSize * 8);
    // printf("%p %p %p %p %p\n", disk, iBmap, dBmap, iBlock, dBlock);
    *(char*)iBmap = 1;			//	reserved : not use
    *(char*)(iBmap + 1) = 1;	//	bad block
    // printf("%hhd %hhd %hhd %hhd\n", *(char*)iBmap, *(char*)(iBmap + 1), *(char*)dBmap, *(char*)(disk + 262143));
    makeRoot();			//	root directory 초기화

}

void makeRoot(){
    Entity* root = (Entity*)calloc(80, sizeof(Entity));		//	4byte짜리 80개 할당
    dBlock = root;
    // printf("%p %p\n", root, dBlock);
    iNode* n = (iNode*)malloc(sizeof(iNode));
    // printf("%p\n", n);
    n->fsize = 320;			//	root directory의 file size : 4*80
    n->blocks = 1;			//	root directory의 block의 개수 : 1개
    n->pointer[0] = 0;		//	root directory가 쓰는 block의 위치 : 0번 index
    *(iNode*)(iBlock + iNodeSize * 2) = *n;
    *(char*)(iBmap + 2) = 1;	//	root directory inode bmap
    *(char*)dBmap = 1;			//	root directory data block bmap
}

void write(char* fileName, int size){
    // printf("write : %s %d\n\n\n", fileName, size);
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
    // printf("inum : %hhd, str : %s\n", ((Entity*)(dBlock + entityIdx))->inum, ((Entity*)(dBlock + entityIdx))->fName);
    iNode *in = (iNode*)malloc(sizeof(iNode));
    in->fsize = size;
    // printf("size : %d, fsize : %d\n", size, in->fsize);
    in->blocks = size / blockSize;
    if(size % blockSize)
        in->blocks++;
    int i;
    // printf("%s, blocks : %d\n", fileName, in->blocks);
    int arr[in->blocks];
    for(i = 0; i < in->blocks; i++){
        char temp = getDataBlockIdx();
        // printf("%s, dBlockIdx : %d\n", fileName, temp);
        if(temp == -1){
            printf("No space!\n");
            break;
        }
        arr[i] = (int)temp;
        // printf("arr[%d] : %d\n",i , arr[i]);
    }
    for(i = 0; i < in->blocks;i++){
        in->pointer[i] = arr[i];
        // printf("%s pointer : %d\n", fileName, in->pointer[i]);
    }
    *(iNode*)(iBlock + inodeIdx * iNodeSize) = *in;
    // printf("target word : %c\n", e->fName[0]);
    for(i = 0; i < size; i++){			//	datablock에 file name의 앞글자를 size 개 만큼 입력
        // printf("%c ", e->fName[0]);
        *(char*)(dBlock + in->pointer[0] * blockSize + i) = e->fName[0];
        // printf("%d\n", i);
        // printf("%c ", *(char*)(dBlock + in->pointer[0] * blockSize + i));
    }
    // printf("%d\n", i);
    // printf("\n");
}

void read(char* fileName, int size){
    // printf("read : %s %d\n", fileName, size);
    int entityIdx = isExist(fileName);
    // printf("entityIdx : %d\n", entityIdx);
    if(entityIdx == -1) {
        printf("No such file\n");
        return;
    }
    char iNum = ((Entity*)(dBlock + entityIdx * entitySize))->inum;
    // printf("iNum : %hhd\n", iNum);
    iNode* in = (iNode*)(iBlock + iNum * iNodeSize);
    int fileSize = in->fsize;
    // printf("fileSize : %d\n", fileSize);
    int i;
    // printf("%d, %d\n", fileSize, size);
    if(fileSize < size){
        // printf("wtf\n");
        for(i = 0; i < fileSize; i++){
            // printf("%p\n", (char*)(dBlock + in->pointer[0] * blockSize + i));
            printf("%c", *(char*)(dBlock + in->pointer[0] * blockSize + i));
        }
        printf("\n");
    }else{
        // printf("WTF\n");
        for(i = 0; i < size; i++)
            printf("%c", *(char*)(dBlock + in->pointer[0] * blockSize + i));
        printf("\n");
    }
}

void delete(char* fileName){
    // printf("delete : %s\n", fileName);int flag = isExist(fileName);
    int entityIdx = isExist(fileName);
    if(entityIdx == -1)
        printf("No such file\n");
    ((Entity*)(dBlock + entityIdx * 4))->inum = 0;
}

char getEntityIdx(){
    // printf("getEntityIdx\n");
    int i = 0;
    for(i = 0; i < 80; i+=entitySize){
        // printf("%d번 : %hhd\n", i, *(char*)(dBlock + i));
        if((*(char*)(dBlock + i)) == 0){
            // printf("%d번 : %hhd\n", i, *(char*)(dBlock + i));
            return (char)i;
        }
    }
    return -1;
}

char getINodeIdx(){
    int i;
    // printf("getINodeIdx\n");
    for(i = 0; i < 56; i++){
        if(*(char*)(iBmap + i) == 0){
            // printf("%d번 : %hhd\n", i, *(char*)(iBmap + i));
            *(char*)(iBmap + i) = 1;
            // printf("%d번 : %hhd\n", i, *(char*)(iBmap + i));
            return (char)i;
        }
    }
    return -1;
}

char getDataBlockIdx(){
    int i;
    // printf("getDataBlockIdx\n");
    for(i = 1; i < 56; i++){
        if((*(char*)(dBmap + i)) == 0){
            // printf("%d번 : %hhd\n", i, (*(char*)(dBmap + i)));
            (*(char*)(dBmap + i)) = 1;
            // printf("%d번 : %hhd\n", i, (*(char*)(dBmap + i)));
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
        // printf("%d : %s\n", i, ((Entity*)(dBlock + i))->fName);
        // printf("결과 : %d\n", strcmp(((Entity*)(dBlock + i))->fName, fileName));
    }
    return -1;		//	없음
}

void printibmap(){
    int i, j, k;
    printf("i-bmap\n");
    for(i = 0; i < 16; i++){
        printf("[%d] || ", i);
        for(j = 0; j < 4; j++){
            for(k = 0; k < 8; k++){
                printf("%hhd", *(char*)(iBmap + i + j + k));
            }
            printf("  ");
        }
        printf("\n");
    }
}

void printdbmap(){
    int i, j, k;
    printf("d-bmap\n");
    for(i = 0; i < 16; i++){
        printf("[%d] || ", i);
        for(j = 0; j < 4; j++){
            for(k = 0; k < 8; k++){
                printf("%hhd", *(char*)(dBmap + i + j + k));
            }
            printf("  ");
        }
        printf("\n");
    }
}

void printiblock(){

}

void printdblock(){

}