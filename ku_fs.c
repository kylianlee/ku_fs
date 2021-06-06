//
// Created by Kylian Lee on 2021/06/06.
//

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
  void* root = calloc(80, 4);		//	4byte짜리 80개 할당
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
  // printf("write : %s %d\n", fileName, size);
  if(isExist(fileName) != -1){
    printf("Already exists\n");
    return;
  }
  char entityIdx = getEntityIdx();
  char inodeIdx = getINodeIdx();
  char dataIdx = getDataBlockIdx();
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
  in->blocks = size / blockSize;
  if(size % blockSize)
    in->blocks++;
  int i;
  // printf("%s, blocks : %d\n", fileName, in->blocks);
  for(i = 0; i < in->blocks; i++){
    char temp = getDataBlockIdx();
    // printf("%s, dBlockIdx : %d\n", fileName, temp);
    in->pointer[i] = temp;
  }
  *(iNode*)(iBlock + inodeIdx * iNodeSize) = *in;
  // printf("%d\n", dataIdx);
  for(i = 0; i < size; i++){			//	datablock에 file name의 앞글자를 size 개 만큼 입력
    *(char*)(dBlock + in->pointer[0] * blockSize + i) = e->fName[0];
    // printf("%c", *(char*)(dBlock + dataIdx * blockSize + i));
  }
}

void read(char* fileName, int size){
  // printf("read : %s %d\n", fileName, size);
  int entityIdx = isExist(fileName);
  // printf("entityIdx : %d\n", entityIdx);
  if(entityIdx == -1)
    printf("No such file\n");
  char iNum = ((Entity*)(dBlock + entityIdx * 4))->inum;
  // printf("iNum : %d\n", iNum);
  char fileSize = ((iNode*)(iBlock + iNum * iNodeSize))->fsize;
  // printf("fileSize : %d\n", fileSize);
  int i;
  // printf("%d, %d\n", fileSize, size);
  if(fileSize < size){
    // printf("pointer : %d\n", ((iNode*)(iBlock + iNum * iNodeSize))->pointer[0]);
    for(i = 0; i < fileSize; i++)
      printf("%c", *(char*)(dBlock + (((iNode*)(iBlock + iNum * iNodeSize))->pointer[0]) * blockSize));
    printf("\n");
  }else{
    // printf("pointer : %d\n", ((iNode*)(iBlock + iNum * iNodeSize))->pointer[0]);
    for(i = 0; i < size; i++)
      printf("%c", *(char*)(dBlock + (((iNode*)(iBlock + iNum * iNodeSize))->pointer[0]) * blockSize));
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
  int i = 0;
  for(i = 0; i < 80; i+=entitySize){
    if(*(char*)(dBlock + i) == 0)
      return (char)i;
  }
  return -1;
}

char getINodeIdx(){
  int i;
  for(i = 0; i < 55; i++){
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
    if(*(char*)(dBmap + i) == 0){
      *(char*)(dBmap + i) = 1;
      return (char)i;
    }
  }
  return -1;
}

int isExist(char* fileName){
  int i;
  for(i = 0; i < 80; i+=entitySize){
    if(((Entity*)(dBlock + i))->inum == 0)
      continue;
    // printf("%d : %s\n", i, ((Entity*)(dBlock + i))->fName);
    // printf("결과 : %d\n", strcmp(((Entity*)(dBlock + i))->fName, fileName));
    if(strcmp(((Entity*)(dBlock + i))->fName, fileName) == 0){
      return i;		//	있는 번호
    }
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