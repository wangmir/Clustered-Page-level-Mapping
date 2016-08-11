#ifndef __dftl_h__
#define __dftl_h__

#include "main.h"

#define SHOULD_GC -1
#define MAP_FB_LIMIT 1

int MAP_SIZE;
int MAP_ON_PAGE;
int current_PB;
int current_MAP_PB;
int MAX_MAP_PB;

typedef struct{
	int Phy_blk;
	int offset;
}PAGE_MAP;

typedef struct{
	int invalid_info;
	int valid_info;
	int top;
	int map_flag;
}PB_INFO;

PAGE_MAP *LPN2PPN;
PB_INFO *PB_info;

int CMT_hit_cnt;
int CMT_miss_cnt;
int shortcut_gc_cnt;

int *MAP_FB_stack;
int MAP_FB_top;

int inGC;

void dftl(char *addr);
void dftl_initializing();
void flash_dftl_write(int start, int cnt);
void flash_dftl_read(int start, int cnt);
void flash_dftl_discard(int start, int cnt);
int request_MAP_FB();
void flash_dftl_map_write(int start);
void falsh_dftl_map_read(int start);
void CMT_function(int LPN, int *);
void DFTL_GC();
void DFTL_map_GC();
void DFTL_freeing();

#endif
