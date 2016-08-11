#ifndef __unit_level_mapping_h__
#define __unit_level_mapping_h__

#include "main.h"

#define AL 1
#define UL 0
#define TOP 1
#define END 0


int BLOCK_per_UNIT;
int PAGE_per_UNIT;
int UNIT_per_FLASH;
int UNIT_per_CMT;

int UNIT_per_FLASH;
int LOGICAL_FLASH_UNIT;
int UBN_MAP_SIZE;
int LPN2UBN_SIZE;
int UBN_offset_SIZE;
int LUN_MAP_SIZE;
int UBN_CMT_PCT;
int LUN_CMT_PCT;
int UBN_CMT_SIZE;
int LUN_CMT_SIZE;
int UBN_ON_PAGE;
int LUN_PAGE_no;
int LUN_ON_PAGE;
int MAX_PB_per_UNIT;
int PB_per_UNIT_ratio;
int UBN_MTB_PAGE;
int LUN_MTB_PAGE;


typedef struct
{
	int LPN2UBN;
	int LPN2offset;

} UBN_MAP;

typedef struct UBN_info *UBN_ptr;

typedef struct UBN_info
{
	int UBN_num;
	int PBN_num;
	int invalid_info;
	int valid_info;
	int top;
	UBN_ptr next;
	
}UBN_info;

typedef struct
{
	UBN_ptr AL_top; //available queue top pointer
	UBN_ptr UL_top; //unavailable qeue top pointer
	UBN_ptr *UBN;
	//int unit_valid_info;
	int unit_invalid_info;
	int intergc_cnt;
	int intragc_cnt;
} LUN_MAP;

typedef struct{
	int UBN_num;
	int UBN_invalid;
}UBN_valid_info;

typedef struct{
	int read;
	int write;
	int erase;
}partial_cnt_;

typedef struct{
	int ifMAP; //for map cluster
	int free_page;
	int inter_GC_cnt;
	int intra_GC_cnt;
	int blk_alloc_cnt;
	partial_cnt_ intraGC;
	partial_cnt_ interGC;
}CL_INFO;

CL_INFO *cl_info;

UBN_MAP *UBN_tb;
LUN_MAP *LUN_tb;

int map_io_flag;

int max_unit;
int inter_GC_cnt;
int shortcut_inter_cnt;
int intra_GC_cnt;
int maxblk_ulm_intra_cnt;
int shortcut_intra_cnt;
int on_gc_unit;

int UBN_hit_cnt;
int UBN_miss_cnt;
int LUN_hit_cnt;
int LUN_miss_cnt;

unsigned long long tot_remain_space;

void unit_level_mapping(char *addr);
void ULM_initializing();
void flash_ulm_write(int start, int cnt);
void flash_ulm_read(int start, int cnt);
void flash_ulm_discard(int start, int cnt);
void UBN_CMT_function(int LPN, int *);
void LUN_CMT_function(int LUN, int *);
void UBN_list_PUSH(int AL_UL, int TOP_END, int LUN, int UBN);
void intra_GC(int LUN);
void inter_GC();
void freeing();


#endif	