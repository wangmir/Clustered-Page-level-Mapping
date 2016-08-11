#ifndef main_h_
#define main_h_

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "unit_level_mapping.h"
#include "dftl.h"
#include "random.h"

#define KB 1024
#define MB KB*1024
#define GB MB*1024
#define SECTOR_SIZE  512
#define FB_LIMIT 20
#define ULM 1
#define DFTL 2
#define INITIAL_WRITE 1
#define INITIAL_SEQUENTIAL_WRITE 2
#define INITIAL_RANDOM_WRITE 3
#define INITIAL_SEQRAND_WRITE 4

#define TRUE 1
#define FALSE 0
/*
//operation 
#define READ 0
#define WRITE 1
#define ERASE 2

//layer
#define IO 0
#define CMT 1
#define GC 2
*/
int FTL_FLAG;

int SECTOR_per_PAGE;
int PAGE_SIZE;
int PAGE_per_BLOCK;
int BLOCK_SIZE;
int CMT_SIZE;
int FLASH_SIZE;
int LOGICAL_FLASH_PAGE;
int LOGICAL_FLASH_BLOCK;
int OVERPROVISIONING_FLASH_BLOCK;
int FLASH_SECTOR;
int FLASH_PAGE;
int FLASH_MTB_SIZE;
int FLASH_MTB_PAGE;
int FLASH_MTB_BLOCK;
int BLOCK_per_FLASH;
int PAGE_per_FLASH;
short int VALID_WATCH_FLAG;
int TRACE_CUTTER;

typedef struct
{
	int			dwIndex;
	float		fTime;			
	int			dwDevId;		
	short int	bRead;	
#define MMC_WRITE 0
#define MMC_READ 1
#define MMC_ERASE_START 2
#define MMC_ERASE_END 3
#define MMC_ERASE 4
	int    dwPageNo;	
	int		dwSectNo;
	int			dwSectCount;
	int			dwCount;		
}IOData;


typedef struct list *list_ptr;

typedef struct list{

	int data;
	list_ptr next;

}list;

typedef struct
{
	int *valid;
	int *PPN2LPN;
} PBN_MAP;

typedef struct{
	int read;
	int write;
	int erase;
}partial_cnt;

PBN_MAP *PBN;

int *FB_stack;
int FB_top;

char *trace_addr;
char *initializing_trace_addr;

int readcnt;
int writecnt;
int erasecnt;
int request_cnt;

int SingleRequestWrite;
int SingleRequestRead;

int SingleRequestCMTWrite;
int SingleRequestCMTRead;

int SingleRequestGCWrite;
int SingleRequestGCRead;
int SingleRequestGCErase;

int SinglePageWrite;
int SinglePageRead;

int SinglePageCMTWrite;
int SinglePageCMTRead;

int SinglePageGCWrite;
int SinglePageGCRead;
int SinglePageGCErase;

int request_count_for_IO_dbg;

FILE *SinglePageAnalysisptr;


list_ptr UBN_link_front;
int UBN_CMT_cnt;
list_ptr LUN_link_front;
int LUN_CMT_cnt;

list_ptr CMT_link_front;
int CMT_cnt;

partial_cnt IO;
partial_cnt IO_at_top;
partial_cnt response;
partial_cnt response_IO;
partial_cnt response_GC;
partial_cnt response_MAP;
partial_cnt response_MAP_inGC;

int initializing_flag;

int discard_start;

void print_welcome();
int flash_initialize(FILE *fp);
void command_setting(int argc, char *argv[]);
void data_unlogging(FILE *fp);
short int read_log(FILE *, void *);
void flash_write(int start, int cnt);
void flash_read(int start, int cnt);
void flash_discard_start(int start, int cnt);
void flash_discard_end(int start, int cnt);
//void Count(int Operation, int Layer);
int request_FB();
list_ptr create_node();
int _log2(int base);
int compare_int(const void *, const void *);
void evaluation(char *);
void valid_watcher(char *);

#endif
