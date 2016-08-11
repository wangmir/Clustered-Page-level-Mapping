
#include "main.h"
#include "unit_level_mapping.h"

extern PBN_MAP *PBN;

partial_cnt intra;
partial_cnt inter;
partial_cnt LUN_func;
partial_cnt UBN_func;

void unit_level_mapping(char *addr){

	FILE *fp;
	
	fp = fopen(addr, "rt");
	assert(fp);


	ULM_initializing();
	data_unlogging(fp);
	fclose(fp);
}

void ULM_initializing(){

	int i;
	int j;

	readcnt = 0;
	writecnt = 0;
	erasecnt = 0;
	inter_GC_cnt = 0;
	intra_GC_cnt = 0;
	maxblk_ulm_intra_cnt = 0;
	intra.erase = 0;
	intra.read = 0;
	intra.write = 0;
	inter.erase = 0;
	inter.read = 0;
	inter.write = 0;
	LUN_func.erase = 0;
	LUN_func.read = 0;
	LUN_func.write =0;
	UBN_func.erase = 0;
	UBN_func.read = 0;
	UBN_func.write = 0;
	IO.erase = 0;
	IO.write = 0;
	IO.read = 0;
	
	UBN_link_front = NULL;
	LUN_link_front = NULL;
	UBN_CMT_cnt = 0;
	LUN_CMT_cnt = 0;

	UBN_hit_cnt = 0;
	UBN_miss_cnt = 0;
	LUN_hit_cnt = 0;
	LUN_miss_cnt = 0;

	shortcut_inter_cnt = 0;
	shortcut_intra_cnt = 0;

	//FLASH_SECTOR = GB / SECTOR_SIZE * FLASH_SIZE;
	//FLASH_PAGE = FLASH_SECTOR / SECTOR_per_PAGE;

	BLOCK_per_FLASH = LOGICAL_FLASH_BLOCK + OVERPROVISIONING_FLASH_BLOCK;
	FLASH_PAGE = PAGE_per_BLOCK * BLOCK_per_FLASH;
	FLASH_SECTOR = FLASH_PAGE * SECTOR_per_PAGE;

	//BLOCK_per_FLASH = FLASH_SECTOR / (SECTOR_per_PAGE * PAGE_per_BLOCK);
	PAGE_per_FLASH = BLOCK_per_FLASH * PAGE_per_BLOCK;
	UNIT_per_FLASH = FLASH_SECTOR / (SECTOR_per_PAGE * PAGE_per_UNIT);
	
	LPN2UBN_SIZE = _log2(BLOCK_per_UNIT);  //in bits
	UBN_offset_SIZE = _log2(PAGE_per_BLOCK); //in bits
	UBN_MAP_SIZE = LPN2UBN_SIZE + UBN_offset_SIZE; //in bits


	if(UBN_MAP_SIZE % 8 == 0)
		UBN_MAP_SIZE = UBN_MAP_SIZE / 8; //in bytes
	else
		UBN_MAP_SIZE = UBN_MAP_SIZE / 8 + 1; //in bytes
	
	//UBN MAP size fix to 4bytes
	UBN_MAP_SIZE = 4;

	LUN_MAP_SIZE = _log2(BLOCK_per_FLASH) + _log2(MAX_PB_per_UNIT * PAGE_per_BLOCK); //in bits with one segments
	if(LUN_MAP_SIZE%8 == 0)
		LUN_MAP_SIZE = LUN_MAP_SIZE/8; //in bytes with one segments
	else
		LUN_MAP_SIZE = LUN_MAP_SIZE/8 + 1;//in bytes with one segments
	LUN_MAP_SIZE = MAX_PB_per_UNIT * LUN_MAP_SIZE;
	UBN_ON_PAGE = PAGE_SIZE / UBN_MAP_SIZE;
	LUN_ON_PAGE = 1;
	if(LUN_MAP_SIZE < PAGE_SIZE){
		LUN_PAGE_no = 1;
		LUN_ON_PAGE = PAGE_SIZE/ LUN_MAP_SIZE;
	}//LU map을 한 페이지에 하나씩 넣을 것인가 아니면 여러개 넣을 수 있다면 여러개 넣을것인가
	else if(LUN_MAP_SIZE%PAGE_SIZE == 0)
		LUN_PAGE_no = LUN_MAP_SIZE/PAGE_SIZE;
	else
		LUN_PAGE_no = LUN_MAP_SIZE/PAGE_SIZE + 1;
	
	FLASH_MTB_SIZE = PAGE_per_FLASH * UBN_MAP_SIZE + UNIT_per_FLASH 
		* LUN_MAP_SIZE;
	UBN_MTB_PAGE = PAGE_per_FLASH/UBN_ON_PAGE;
	LUN_MTB_PAGE = UNIT_per_FLASH/LUN_ON_PAGE*LUN_PAGE_no;
	FLASH_MTB_PAGE = UBN_MTB_PAGE + LUN_MTB_PAGE;

	LOGICAL_FLASH_BLOCK = LOGICAL_FLASH_BLOCK -
		(FLASH_MTB_PAGE % PAGE_per_BLOCK ? (FLASH_MTB_PAGE / PAGE_per_BLOCK + 1) : (FLASH_MTB_PAGE / PAGE_per_BLOCK));
	LOGICAL_FLASH_PAGE = LOGICAL_FLASH_BLOCK * PAGE_per_BLOCK;

	//LOGICAL_FLASH_PAGE = (FLASH_PAGE - FLASH_MTB_PAGE)*97/100;
	
	LOGICAL_FLASH_UNIT = LOGICAL_FLASH_PAGE/PAGE_per_UNIT;
	/*
	UBN_CMT_SIZE = CMT_SIZE * UBN_CMT_PCT / 100;
	LUN_CMT_SIZE = CMT_SIZE * LUN_CMT_PCT / 100;
	*/
	LUN_CMT_SIZE = 5 * PAGE_SIZE;
	UBN_CMT_SIZE = CMT_SIZE - LUN_CMT_SIZE;
	//LUN이 맵에 항상 상주하도록 변경
#if 0
	LUN_CMT_SIZE = LUN_MTB_PAGE * PAGE_SIZE;
	UBN_CMT_SIZE = CMT_SIZE - LUN_CMT_SIZE;
#endif
	UBN_tb = (UBN_MAP *)malloc(sizeof(UBN_MAP)*PAGE_per_FLASH);
	LUN_tb = (LUN_MAP *)malloc(sizeof(LUN_MAP)*UNIT_per_FLASH);
	PBN = (PBN_MAP *)malloc(sizeof(PBN_MAP) * BLOCK_per_FLASH);

	for(i=0; i<PAGE_per_FLASH; i++){

		UBN_tb[i].LPN2UBN = -1;
		UBN_tb[i].LPN2offset = -1;
	}

	for(i=0; i<UNIT_per_FLASH; i++){

		LUN_tb[i].UBN = (UBN_ptr *)malloc(sizeof(UBN_ptr)*MAX_PB_per_UNIT);
		LUN_tb[i].AL_top = NULL;
		LUN_tb[i].UL_top = NULL;
		//LUN_tb[i].unit_valid_info = 0;
		LUN_tb[i].unit_invalid_info = 0;
		LUN_tb[i].intragc_cnt= 0;
		LUN_tb[i].intergc_cnt = 0;
		for(j=0; j<MAX_PB_per_UNIT ; j++){
			LUN_tb[i].UBN[j] = (UBN_info *)malloc(sizeof(UBN_info));
			LUN_tb[i].UBN[j]->invalid_info = 0;
			LUN_tb[i].UBN[j]->PBN_num = -1;
			LUN_tb[i].UBN[j]->top = -1;
			LUN_tb[i].UBN[j]->UBN_num = j;
			LUN_tb[i].UBN[j]->valid_info = 0;
		}
		LUN_tb[i].AL_top = LUN_tb[i].UBN[0];
		for(j=0; j<MAX_PB_per_UNIT - 1 ; j++){
			LUN_tb[i].UBN[j]->next = LUN_tb[i].UBN[j+1];		
		}
		LUN_tb[i].UBN[MAX_PB_per_UNIT - 1]->next = NULL;
	}

	FB_stack = (int *)malloc(sizeof(int)*BLOCK_per_FLASH);
	FB_top = -1;
	

	for(i=0; i<BLOCK_per_FLASH; i++){

		FB_stack[i] = i;
		FB_top++;
		PBN[i].PPN2LPN = (int *)malloc(sizeof(int) * PAGE_per_BLOCK);
		PBN[i].valid = (int *)malloc(sizeof(int) * PAGE_per_BLOCK);
		for(j=0; j<PAGE_per_BLOCK ; j++){
			PBN[i].PPN2LPN[j] = -1;
			PBN[i].valid[j] = -1;
		}
	}

	cl_info = (CL_INFO *) malloc(sizeof(CL_INFO) * UNIT_per_FLASH);
	memset(cl_info, 0x00, sizeof(CL_INFO) * UNIT_per_FLASH);

	printf("\ninitialize end\n");
	
}

static void check_LUN_tb(){

	int iter;

	for (iter = 0; iter < UNIT_per_FLASH; iter++){
		
		if (LUN_tb[iter].AL_top == NULL && LUN_tb[iter].UL_top == NULL)
			iter = iter;
	}
}


void flash_ulm_write(int start, int cnt){

	int i,j;
	int *pLUN_int; 
	int	*pUBN_int;
	int LUN_int;
	int UBN_int;
	int UBN_no;
	int max;
	int intra_GC_flag = 0;
	UBN_ptr temp;
	UBN_ptr post = NULL;

	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;
	

	//printf("\n%d %d", start, cnt);

	if((PAGE_per_UNIT - start%PAGE_per_UNIT) < cnt){

		flash_write(start, PAGE_per_UNIT - start%PAGE_per_UNIT);
		flash_write(start + PAGE_per_UNIT - start%PAGE_per_UNIT,
			cnt - (PAGE_per_UNIT - start%PAGE_per_UNIT));
	}
	else{
		LUN_CMT_function(start/PAGE_per_UNIT, pLUN_int);

		for(i=0; i<cnt ; i++){

			UBN_CMT_function(start+i, pUBN_int);

			if(LUN_tb[*pLUN_int].AL_top == NULL){
				intra_GC(*pLUN_int);
				intra_GC_flag = 1;
			}
			if((LUN_tb[*pLUN_int].AL_top->top == -1) && (intra_GC_flag == 0)){
				if(FB_top <= FB_LIMIT){
					if(LUN_tb[*pLUN_int].UL_top != NULL){
						if(LUN_tb[*pLUN_int].UL_top->invalid_info == PAGE_per_BLOCK){
							intra_GC(*pLUN_int);
							intra_GC_flag = 1;
						}
						else{
							max = LUN_tb[0].unit_invalid_info;
							max_unit = 0;
							for(j=0; j<LOGICAL_FLASH_UNIT; j++){
								if(j == on_gc_unit)
									continue;
 								if(LUN_tb[j].unit_invalid_info > max){
									max_unit = j;
									max = LUN_tb[j].unit_invalid_info;
								}
							}
							if(LUN_tb[max_unit].unit_invalid_info<PAGE_per_BLOCK)
								printf("!!:Maximum unit has no more invalid page than PAGE_per_BLOCK\n");
							if(max_unit == *pLUN_int){
								intra_GC(*pLUN_int);
								intra_GC_flag = 1;
							}
							else{
								LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB(); //top이 다차지도않았는데 request
								cl_info[*pLUN_int].blk_alloc_cnt++;
							}
						}
					}
					else{
						max = LUN_tb[0].unit_invalid_info;
						max_unit = 0;
						for(j=0; j<LOGICAL_FLASH_UNIT; j++){ //LOGICAL_FLASH_UNIT으로 했을때 프리블록이 맵 유닛에 몰리는 현상발생
							if(j == on_gc_unit)			//UNIT_per_FLASH로 했을 때 에러 발생
								continue;
							if(LUN_tb[j].unit_invalid_info > max){
								max_unit = j;
								max = LUN_tb[j].unit_invalid_info;
							}
						}
						if(LUN_tb[max_unit].unit_invalid_info<PAGE_per_BLOCK)
								printf("ERROR: invalid infor is lower than single block\n");
						if(max_unit == *pLUN_int){
							intra_GC(*pLUN_int);
							intra_GC_flag = 1;
						}
						else{
							LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB();
							cl_info[*pLUN_int].blk_alloc_cnt++;
						}
					}
				}
				else{
					LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB();
					cl_info[*pLUN_int].blk_alloc_cnt++;
				}
			}
			if(intra_GC_flag == 1)
				intra_GC_flag = 0;
			if(UBN_tb[*pUBN_int].LPN2UBN == -1){
				UBN_tb[*pUBN_int].LPN2UBN = LUN_tb[*pLUN_int].AL_top->UBN_num;
				LUN_tb[*pLUN_int].AL_top->top++;
				UBN_tb[*pUBN_int].LPN2offset = LUN_tb[*pLUN_int].AL_top->top;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].PPN2LPN[LUN_tb[*pLUN_int].AL_top->top] = *pUBN_int;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].valid[LUN_tb[*pLUN_int].AL_top->top] = 1;
				writecnt++;
				if(map_io_flag != 1)
					IO.write++;
				if(LUN_tb[*pLUN_int].AL_top->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info += LUN_tb[*pLUN_int].AL_top->invalid_info;
					temp = LUN_tb[*pLUN_int].AL_top;
					post = LUN_tb[*pLUN_int].AL_top->next;
					LUN_tb[*pLUN_int].AL_top = post;
					UBN_list_PUSH(UL, END, *pLUN_int, temp->UBN_num);

				}
			}
			else{ 
				UBN_no = UBN_tb[*pUBN_int].LPN2UBN;
				PBN[LUN_tb[*pLUN_int].UBN[UBN_no]->PBN_num].valid[UBN_tb[*pUBN_int].LPN2offset] = 0;
				LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info++;
				if(LUN_tb[*pLUN_int].UBN[UBN_no]->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info++;
				}
				if(LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info==PAGE_per_BLOCK){
					temp = LUN_tb[*pLUN_int].UL_top;
					while(1){
						if(temp->UBN_num == UBN_no){
							if(temp != LUN_tb[*pLUN_int].UL_top){
								post->next = temp->next;
								UBN_list_PUSH(UL, TOP, *pLUN_int, temp->UBN_num);
							}
							break;
						}
						else{
							post = temp;
							temp = post->next;
							continue;
						}
					}
				}
				UBN_tb[*pUBN_int].LPN2UBN = LUN_tb[*pLUN_int].AL_top->UBN_num;
				LUN_tb[*pLUN_int].AL_top->top++;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].PPN2LPN[LUN_tb[*pLUN_int].AL_top->top] = *pUBN_int;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].valid[LUN_tb[*pLUN_int].AL_top->top] = 1;
				UBN_tb[*pUBN_int].LPN2offset = LUN_tb[*pLUN_int].AL_top->top;
				writecnt++;
				if(map_io_flag != 1)
					IO.write++;
				if(LUN_tb[*pLUN_int].AL_top->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info += LUN_tb[*pLUN_int].AL_top->invalid_info;
					temp = LUN_tb[*pLUN_int].AL_top;
					post = LUN_tb[*pLUN_int].AL_top->next;
					LUN_tb[*pLUN_int].AL_top = post;
					UBN_list_PUSH(UL, END, *pLUN_int, temp->UBN_num);
				}
			}
			//write operation
			//unit invalid info 에는 UL에 들어있는 블록의 invalid info만 포함되어야한다.
		}
	}
	check_LUN_tb();
}

void flash_ulm_read(int start, int cnt){
	
	int *pLUN_int;
	int *pUBN_int;
	int LUN_int;
	int UBN_int;
	int i;
	
	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;



	if((PAGE_per_UNIT - start%PAGE_per_UNIT) < cnt){

		flash_read(start, PAGE_per_UNIT - start%PAGE_per_UNIT);
		flash_read(start + PAGE_per_UNIT - start%PAGE_per_UNIT,
			cnt - PAGE_per_UNIT - start%PAGE_per_UNIT);
	}
	else{
		//LUN_CMT_function(start/PAGE_per_UNIT, pLUN_int);
		for(i=0; i<cnt; i++){
			UBN_CMT_function(start+i, pUBN_int);
			readcnt++;
			IO.read++;
		}
	}
}

void flash_ulm_discard(int start, int cnt){


	int i, j;
	int *pLUN_int;
	int	*pUBN_int;
	int LUN_int;
	int UBN_int;
	int UBN_no;
	int max;
	int intra_GC_flag = 0;
	UBN_ptr temp;
	UBN_ptr post = NULL;

	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;

	if ((PAGE_per_UNIT - start%PAGE_per_UNIT) < cnt){

		flash_ulm_discard(start, PAGE_per_UNIT - start%PAGE_per_UNIT);
		flash_ulm_discard(start + PAGE_per_UNIT - start%PAGE_per_UNIT,
			cnt - (PAGE_per_UNIT - start%PAGE_per_UNIT));
	}
	else{

		LUN_CMT_function(start / PAGE_per_UNIT, pLUN_int);

		for (i = 0; i < cnt; i++){
			UBN_CMT_function(start + i, pUBN_int);
			if (UBN_tb[*pUBN_int].LPN2UBN == -1){
				return;
			}
			else{
				UBN_no = UBN_tb[*pUBN_int].LPN2UBN;
				PBN[LUN_tb[*pLUN_int].UBN[UBN_no]->PBN_num].valid[UBN_tb[*pUBN_int].LPN2offset] = 0;
				LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info++;
				if (LUN_tb[*pLUN_int].UBN[UBN_no]->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info++;
				}
				if (LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info == PAGE_per_BLOCK){
					temp = LUN_tb[*pLUN_int].UL_top;
					while (1){
						if (temp->UBN_num == UBN_no){
							if (temp != LUN_tb[*pLUN_int].UL_top){
								post->next = temp->next;
								UBN_list_PUSH(UL, TOP, *pLUN_int, temp->UBN_num);
							}
							break;
						}
						else{
							post = temp;
							temp = post->next;
							continue;
						}
					}
				}
				UBN_tb[*pUBN_int].LPN2UBN = -1;
			}
		}
	}
}


static void flash_ulm_map_write(int start, int cnt){

	int i, j;
	int *pLUN_int;
	int	*pUBN_int;
	int LUN_int;
	int UBN_int;
	int UBN_no;
	int max;
	int intra_GC_flag = 0;
	UBN_ptr temp;
	UBN_ptr post = NULL;

	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;



	//printf("\n%d %d", start, cnt);

	if ((PAGE_per_UNIT - start%PAGE_per_UNIT) < cnt){

		flash_write(start, PAGE_per_UNIT - start%PAGE_per_UNIT);
		flash_write(start + PAGE_per_UNIT - start%PAGE_per_UNIT,
			cnt - (PAGE_per_UNIT - start%PAGE_per_UNIT));
	}
	else{
		//LUN_CMT_function(start / PAGE_per_UNIT, pLUN_int);
		*pLUN_int = start / PAGE_per_UNIT;

		cl_info[*pLUN_int].ifMAP = 1;

		for (i = 0; i<cnt; i++){

			request_count_for_IO_dbg++;

			//UBN_CMT_function(start + i, pUBN_int);
			*pUBN_int = start + i; 

			if (LUN_tb[*pLUN_int].AL_top == NULL){
				intra_GC(*pLUN_int);
				intra_GC_flag = 1;
			}
			if ((LUN_tb[*pLUN_int].AL_top->top == -1) && (intra_GC_flag == 0)){
				if (FB_top <= FB_LIMIT){
					if (LUN_tb[*pLUN_int].UL_top != NULL){
						if (LUN_tb[*pLUN_int].UL_top->invalid_info == PAGE_per_BLOCK){
							intra_GC(*pLUN_int);
							intra_GC_flag = 1;
						}
						else{
							max = LUN_tb[0].unit_invalid_info;
							max_unit = 0;
							for (j = 0; j<LOGICAL_FLASH_UNIT; j++){
								if (j == on_gc_unit)
									continue;
								if (LUN_tb[j].unit_invalid_info > max){
									max_unit = j;
									max = LUN_tb[j].unit_invalid_info;
								}
							}
							if (LUN_tb[max_unit].unit_invalid_info<PAGE_per_BLOCK)
								printf("ERROR: unit invalid page is lower than single block\n");
							if (max_unit == *pLUN_int){
								intra_GC(*pLUN_int);
								intra_GC_flag = 1;
							}
							else{
								LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB();//top이 다차지도않았는데 request
								cl_info[*pLUN_int].blk_alloc_cnt++;
							}
						}
					}
					else{
						max = LUN_tb[0].unit_invalid_info;
						max_unit = 0;
						for (j = 0; j<LOGICAL_FLASH_UNIT; j++){ //LOGICAL_FLASH_UNIT으로 했을때 프리블록이 맵 유닛에 몰리는 현상발생
							if (j == on_gc_unit)			//UNIT_per_FLASH로 했을 때 에러 발생
								continue;
							if (LUN_tb[j].unit_invalid_info > max){
								max_unit = j;
								max = LUN_tb[j].unit_invalid_info;
							}
						}
						if (LUN_tb[max_unit].unit_invalid_info<PAGE_per_BLOCK)
							printf("ERROR: Unit invalid page is lower than single block\n");
						if (max_unit == *pLUN_int){
							intra_GC(*pLUN_int);
							intra_GC_flag = 1;
						}
						else{
							LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB();
							cl_info[*pLUN_int].blk_alloc_cnt++;
						}
					}
				}
				else{
					LUN_tb[*pLUN_int].AL_top->PBN_num = request_FB();
					cl_info[*pLUN_int].blk_alloc_cnt++;
				}
			}
			if (intra_GC_flag == 1)
				intra_GC_flag = 0;
			if (UBN_tb[*pUBN_int].LPN2UBN == -1){
				UBN_tb[*pUBN_int].LPN2UBN = LUN_tb[*pLUN_int].AL_top->UBN_num;
				LUN_tb[*pLUN_int].AL_top->top++;
				UBN_tb[*pUBN_int].LPN2offset = LUN_tb[*pLUN_int].AL_top->top;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].PPN2LPN[LUN_tb[*pLUN_int].AL_top->top] = *pUBN_int;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].valid[LUN_tb[*pLUN_int].AL_top->top] = 1;
				writecnt++;
				if (LUN_tb[*pLUN_int].AL_top->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info += LUN_tb[*pLUN_int].AL_top->invalid_info;
					temp = LUN_tb[*pLUN_int].AL_top;
					post = LUN_tb[*pLUN_int].AL_top->next;
					LUN_tb[*pLUN_int].AL_top = post;
					UBN_list_PUSH(UL, END, *pLUN_int, temp->UBN_num);

				}
			}
			else{
				UBN_no = UBN_tb[*pUBN_int].LPN2UBN;
				PBN[LUN_tb[*pLUN_int].UBN[UBN_no]->PBN_num].valid[UBN_tb[*pUBN_int].LPN2offset] = 0;
				LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info++;
				if (LUN_tb[*pLUN_int].UBN[UBN_no]->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info++;
				}
				if (LUN_tb[*pLUN_int].UBN[UBN_no]->invalid_info == PAGE_per_BLOCK){
					temp = LUN_tb[*pLUN_int].UL_top;
					while (1){
						if (temp->UBN_num == UBN_no){
							if (temp != LUN_tb[*pLUN_int].UL_top){
								post->next = temp->next;
								UBN_list_PUSH(UL, TOP, *pLUN_int, temp->UBN_num);
							}
							break;
						}
						else{
							post = temp;
							temp = post->next;
							continue;
						}
					}
				}
				UBN_tb[*pUBN_int].LPN2UBN = LUN_tb[*pLUN_int].AL_top->UBN_num;
				LUN_tb[*pLUN_int].AL_top->top++;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].PPN2LPN[LUN_tb[*pLUN_int].AL_top->top] = *pUBN_int;
				PBN[LUN_tb[*pLUN_int].AL_top->PBN_num].valid[LUN_tb[*pLUN_int].AL_top->top] = 1;
				UBN_tb[*pUBN_int].LPN2offset = LUN_tb[*pLUN_int].AL_top->top;
				writecnt++;
				if (LUN_tb[*pLUN_int].AL_top->top == PAGE_per_BLOCK - 1){
					LUN_tb[*pLUN_int].unit_invalid_info += LUN_tb[*pLUN_int].AL_top->invalid_info;
					temp = LUN_tb[*pLUN_int].AL_top;
					post = LUN_tb[*pLUN_int].AL_top->next;
					LUN_tb[*pLUN_int].AL_top = post;
					UBN_list_PUSH(UL, END, *pLUN_int, temp->UBN_num);
				}
			}
			//write operation
			//unit invalid info 에는 UL에 들어있는 블록의 invalid info만 포함되어야한다.
		}
	}
}

void UBN_CMT_function(int LPN_, int *UBN_int){

	list_ptr temp;
	list_ptr post = NULL;
	int LPN;
	short int write_flag = 0;
	int temp1;

	//_CrtMemDumpAllObjectsSince(0);
	temp = UBN_link_front;
	map_io_flag = 0;
	LPN = LOGICAL_FLASH_PAGE + LPN_ / UBN_ON_PAGE;

	if(LPN_ > LOGICAL_FLASH_PAGE){
		*UBN_int = LPN_;
	}
	else{
		while(1){
			if((temp == NULL) || (temp->next==NULL)){
				UBN_miss_cnt++;
				if(((UBN_CMT_cnt+1)*PAGE_SIZE)>UBN_CMT_SIZE){
					temp1 = temp->data;
					write_flag = 1;
					post->next = NULL;
					//flash_read(LPN, 1);
					readcnt++;
					UBN_func.read++;
					temp->next = UBN_link_front;
					UBN_link_front = temp;
					temp->data = LPN;
					*UBN_int = LPN_;
					break;
				}
				else{
					if(temp == NULL){
						temp = create_node();
						UBN_CMT_cnt++;
						//flash_read(LPN, 1);
						readcnt++;
						UBN_func.read++;
						UBN_link_front = temp;
						temp->data = LPN;
						*UBN_int = LPN_;
						break;
					}
					else{
						temp = create_node();
						UBN_CMT_cnt++;
						//flash_read(LPN, 1);
						readcnt++;
						UBN_func.read++;
						temp->next = UBN_link_front;
						UBN_link_front = temp;
						temp->data = LPN;
						*UBN_int = LPN_;
						break;
					}
				}
			}
			else{
				if(temp->data == LPN){
					UBN_hit_cnt++;
					if(temp == UBN_link_front){
						*UBN_int = LPN_;
						break;
					}
					else{
						post->next = temp->next;
						temp->next = UBN_link_front;
						UBN_link_front = temp;
						*UBN_int = LPN_;
						break;
					}
				}
				else{
					post = temp;
					temp = temp->next;
					continue;
				}
			}
		}

		if(write_flag == 1){
			map_io_flag = 1;
			//flash_write(temp1, 1);
			flash_ulm_map_write(temp1, 1);
			map_io_flag = 0;
			UBN_func.write++;
			write_flag = 0;
		}
	}
}

void LUN_CMT_function(int LUN_num, int *LUN_int){

	list_ptr temp;
	list_ptr post = NULL;
	short int write_flag = 0;
	int temp1;
	temp = LUN_link_front;

	map_io_flag = 0;

	if(LUN_num > LOGICAL_FLASH_PAGE/PAGE_per_UNIT){
		*LUN_int = LUN_num;
	}
	else{
		while(1){
			if((temp == NULL) || (temp->next==NULL)){
				LUN_miss_cnt++;
				if(((LUN_CMT_cnt+1)*LUN_MAP_SIZE)>LUN_CMT_SIZE){
					temp1 = LOGICAL_FLASH_PAGE + UBN_MTB_PAGE + temp->data/LUN_ON_PAGE * LUN_PAGE_no;
					write_flag = 1;
					post->next = NULL;
					//flash_read(LOGICAL_FLASH_PAGE + UBN_MTB_PAGE + LUN_num * LUN_PAGE_no, LUN_PAGE_no);
					if(LUN_ON_PAGE > 1)
						readcnt++;
					else
						readcnt = readcnt + LUN_PAGE_no;
					LUN_func.read++;
					temp->next = LUN_link_front;
					LUN_link_front = temp;
					temp->data = LUN_num;
					*LUN_int = LUN_num;
					break;
				}
				else{
					if(temp == NULL){
						temp = create_node();
						LUN_CMT_cnt++;
						//flash_read(LOGICAL_FLASH_PAGE + UBN_MTB_PAGE + LUN_num * LUN_PAGE_no, LUN_PAGE_no);
						if(LUN_ON_PAGE>1)
							readcnt++;
						else
							readcnt = readcnt + LUN_PAGE_no;
						LUN_func.read++;
						LUN_link_front = temp;
						temp->data = LUN_num;
						*LUN_int = LUN_num;//
						break;
					}
					else{
						temp = create_node();
						LUN_CMT_cnt++;
						//flash_read(LOGICAL_FLASH_PAGE + UBN_MTB_PAGE + LUN_num * LUN_PAGE_no, LUN_PAGE_no);
						if(LUN_ON_PAGE>1)
							readcnt++;
						else
							readcnt = readcnt + LUN_PAGE_no;
						LUN_func.read++;
						temp->next = LUN_link_front;
						LUN_link_front = temp;
						temp->data = LUN_num;
						*LUN_int = LUN_num;
						break;
					}
				}
			}
			else{
				if(temp->data == LUN_num){
					LUN_hit_cnt++;
					if(temp == LUN_link_front){
						*LUN_int = LUN_num;
						break;
					}
					else{
						post->next = temp->next;
						temp->next = LUN_link_front;
						LUN_link_front = temp;
						*LUN_int = LUN_num;
						break;
					}
				}
				else{
					post = temp;
					temp = temp->next;
					continue;
				}
			}
		}
		if(write_flag == 1){
			map_io_flag = 1;
			if (LUN_ON_PAGE > 1){
				//flash_write(temp1, 1);
				flash_ulm_map_write(temp1, 1);
				LUN_func.write++;
			}
			else{
				//flash_write(temp1, LUN_PAGE_no);
				flash_ulm_map_write(temp1, LUN_PAGE_no);
				LUN_func.write += LUN_PAGE_no;
			}
			map_io_flag = 0;
			write_flag = 0;
		}
	}
}

void UBN_list_PUSH(int list_flag, int loc_flag, int LUN_no, int UBN_no){

	UBN_ptr temp;
	UBN_ptr post;

	if(list_flag == AL)
		temp = LUN_tb[LUN_no].AL_top;
	else
		temp = LUN_tb[LUN_no].UL_top;

	if(loc_flag == TOP){
		LUN_tb[LUN_no].UBN[UBN_no]->next = temp;
		if(list_flag == AL)
			LUN_tb[LUN_no].AL_top = LUN_tb[LUN_no].UBN[UBN_no];
		else
			LUN_tb[LUN_no].UL_top = LUN_tb[LUN_no].UBN[UBN_no];
	}
	else{
		while(1){
			if(temp == NULL){
				if(list_flag == AL){
					LUN_tb[LUN_no].UBN[UBN_no]->next = NULL;
					LUN_tb[LUN_no].AL_top = LUN_tb[LUN_no].UBN[UBN_no];
				}
				else{
					LUN_tb[LUN_no].UBN[UBN_no]->next = NULL;
					LUN_tb[LUN_no].UL_top = LUN_tb[LUN_no].UBN[UBN_no];
				}
				break;
			}
			else if(temp->next == NULL){
				temp->next = LUN_tb[LUN_no].UBN[UBN_no];
				LUN_tb[LUN_no].UBN[UBN_no]->next = NULL;
				break;
			}
			else{
				post = temp;
				temp = post->next;
				continue;
			}
		}
	}
}

static void calc_remain_space(){
	int i, k = 0, l = 0;
	UBN_ptr temp;

	/*if (FB_top > FB_LIMIT){
		maxblk_ulm_intra_cnt++;
		return;
	}*/

	for (i = 0; i < LOGICAL_FLASH_UNIT; i++)
	{
		temp = LUN_tb[i].AL_top;
		while (1){
			if (temp == NULL)
				break;
			else if (temp->top == -1)
				break;
			else{
				l = PAGE_per_BLOCK - (temp->top + 1);
				if (l < 0)
					continue;
				k += l;
				cl_info[i].free_page += l;
			}
			temp = LUN_tb[i].AL_top->next;
		}
	}
	tot_remain_space += k;
}

void intra_GC(int LUN){

	int *pLUN_int;
	int *pUBN_int;
	int LUN_int;
	int UBN_int;
	UBN_ptr post = NULL;
	UBN_ptr temp;
	int max = 0;
	int i;
	int temp1;
	UBN_ptr max_ptr;
	on_gc_unit = LUN;

	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;

	calc_remain_space();

	temp = LUN_tb[LUN].UL_top;
	intra_GC_cnt++;
	LUN_tb[LUN].intragc_cnt++;

	cl_info[LUN].blk_alloc_cnt++;
	cl_info[LUN].intra_GC_cnt++;

	if(LUN_tb[LUN].UL_top->invalid_info == PAGE_per_BLOCK){

		LUN_tb[LUN].UL_top = temp->next;
		LUN_tb[LUN].unit_invalid_info -= temp->invalid_info;
		temp->invalid_info = 0;
		temp->top = -1;
		temp1 = FB_stack[FB_top];
		FB_stack[FB_top] = temp->PBN_num;
		temp->PBN_num = temp1;
		UBN_list_PUSH(AL, TOP, LUN, temp->UBN_num);//AL list 로 push함으로 active 시킨다.
	} // full invalid block이 있을 경우는 무조건 ul_top에 들어있다.  즉 ul_top을 검사하면 full block을 찾을 수 있고,
	//그에 대해서는 switch merge가 가능하다.
	else{
		max_ptr = LUN_tb[LUN].UL_top;
		while(1){
			if(temp->next == NULL){
				if(temp->invalid_info > max_ptr->invalid_info)
					max_ptr = temp;
				break;
			}
			else{
				if(temp->invalid_info > max_ptr->invalid_info)
					max_ptr = temp;
				post = temp;
				temp = post->next;
				continue;
			}
		}
		temp = LUN_tb[LUN].UL_top;
		while(1){
			if(temp == max_ptr){
				if(temp != LUN_tb[LUN].UL_top){
					post->next = temp->next;
				}
				else{
					LUN_tb[LUN].UL_top = temp->next;
				}
				break;
			}
			else{
				post = temp;
				temp = post->next;
				continue;
			}
		}
		temp1 = FB_stack[FB_top];
		FB_top--;
		max_ptr->top = -1;
		for (i = 0; i < PAGE_per_BLOCK; i++){
			readcnt++;
			intra.read++;
			cl_info[LUN].intraGC.read++;

			if(PBN[max_ptr->PBN_num].valid[i] == 1){
				if (map_io_flag == 0)
					UBN_CMT_function(PBN[max_ptr->PBN_num].PPN2LPN[i], pUBN_int);
				else
					*pUBN_int = PBN[max_ptr->PBN_num].PPN2LPN[i];
				max_ptr->top++;
				PBN[temp1].PPN2LPN[max_ptr->top] = *pUBN_int;
				PBN[temp1].valid[max_ptr->top] = 1;
				UBN_tb[*pUBN_int].LPN2offset = max_ptr->top;
				writecnt++;
				intra.write++;
				cl_info[LUN].intraGC.write++;
			}
		}
		UBN_list_PUSH(AL, TOP, LUN, max_ptr->UBN_num);
		FB_top++;
		FB_stack[FB_top] = max_ptr->PBN_num;
		max_ptr->PBN_num = temp1;
		LUN_tb[LUN].unit_invalid_info -= max_ptr->invalid_info;
		max_ptr->invalid_info = 0;
	}
	erasecnt++;
	intra.erase++;
	cl_info[LUN].intraGC.erase++;

	for(i=0; i<PAGE_per_BLOCK; i++){
 		PBN[FB_stack[FB_top]].PPN2LPN[i] = -1;
		PBN[FB_stack[FB_top]].valid[i] = -1;
	}
	on_gc_unit = -1;
	//intra_GC operation

}

void inter_GC(){

	int i, j;
	int *pLUN_int;
	int *pUBN_int;
	int LUN_int;
	int UBN_int;
	int page_cnt = 0;
	int UBN_no;
	int temp_PB;
	short int virgin_flag = 1;
	UBN_valid_info *UBN_val;
	UBN_ptr temp;
	UBN_ptr post = NULL;

	pLUN_int = &LUN_int;
	pUBN_int = &UBN_int;
	
	calc_remain_space();

	inter_GC_cnt++;
	on_gc_unit = max_unit;
	
	LUN_CMT_function(max_unit, pLUN_int);

	UBN_val = (UBN_valid_info *)malloc(sizeof(UBN_valid_info)*MAX_PB_per_UNIT);
	temp = LUN_tb[*pLUN_int].UL_top;
	LUN_tb[*pLUN_int].intergc_cnt++;
	cl_info[*pLUN_int].inter_GC_cnt++;

	if(temp->invalid_info == PAGE_per_BLOCK){

		FB_top++;
		FB_stack[FB_top] = temp->PBN_num;
		erasecnt++;
		inter.erase++;
		cl_info[*pLUN_int].interGC.erase++;

		LUN_tb[*pLUN_int].unit_invalid_info -= temp->invalid_info;
		temp->invalid_info = 0;
		temp->top = -1;
		LUN_tb[*pLUN_int].UL_top = temp->next;
		temp->PBN_num = -1;
		UBN_list_PUSH(AL, END, *pLUN_int, temp->UBN_num);
		for(i=0; i<PAGE_per_BLOCK; i++){
			PBN[FB_stack[FB_top]].PPN2LPN[i] = -1;
			PBN[FB_stack[FB_top]].valid[i] = -1;
		}
	}
	else{
		for(i=0; i<MAX_PB_per_UNIT;i++){
			UBN_val[i].UBN_invalid = 0;
		}
		i=0;
		while(1){
			UBN_val[i].UBN_num = temp->UBN_num;
			UBN_val[i].UBN_invalid = temp->invalid_info;
			i++;
			if(temp->next == NULL){
				break;
			}
			else{
				post = temp;
				temp = post->next;
				continue;
			}
		}
		qsort(UBN_val, MAX_PB_per_UNIT, sizeof(UBN_valid_info), compare_int);
		for(i=0;;i++){
			if(page_cnt >= PAGE_per_BLOCK)
				break;
			else if (LUN_tb[*pLUN_int].UBN[UBN_val[i].UBN_num]->top != PAGE_per_BLOCK - 1){
				//여기가 utilization이 떨어질 수도 있는 문제.
				//하지만 continue를 하고 있고, 지금까지 문제가 생기지 않음.
				//utilization 문제를 확인 하기 위해 이부분 수정 필요.
				continue;
			}
			page_cnt += UBN_val[i].UBN_invalid;
			UBN_no = UBN_val[i].UBN_num;

			temp = LUN_tb[*pLUN_int].UL_top;
			while(1){
				if(temp->UBN_num == UBN_no){
					if(temp != LUN_tb[*pLUN_int].UL_top){
						post->next = temp->next;
					}
					else{
						LUN_tb[*pLUN_int].UL_top = temp->next;
					}
					break;
				}
				else{
					post = temp;
					temp = post->next;
					continue;
				}
			}
			temp_PB = LUN_tb[*pLUN_int].UBN[UBN_no]->PBN_num;
			temp = LUN_tb[*pLUN_int].UBN[UBN_no];
			temp->top = -1;
			temp->PBN_num = -1;
			LUN_tb[*pLUN_int].unit_invalid_info -= temp->invalid_info;
			temp->invalid_info = 0;
			UBN_list_PUSH(AL, END, *pLUN_int, temp->UBN_num);
			//남아있는 쓰기 영역이 있는데 그냥 다시 top으로 푸시
			for(j=0; j<PAGE_per_BLOCK; j++){
				inter.read++;
				readcnt++;
				cl_info[*pLUN_int].interGC.read++;
				if(PBN[temp_PB].valid[j] == 1){
					inter.write++;
					writecnt++;
					cl_info[*pLUN_int].interGC.write++;
					UBN_CMT_function(PBN[temp_PB].PPN2LPN[j], pUBN_int);
					
					temp = LUN_tb[*pLUN_int].AL_top;
					if(temp->top == -1){
						temp->PBN_num = FB_stack[FB_top];
						FB_top--;
					}
					temp->top++;
					PBN[temp->PBN_num].PPN2LPN[temp->top] = *pUBN_int;
					PBN[temp->PBN_num].valid[temp->top] = 1;
					UBN_tb[*pUBN_int].LPN2UBN = temp->UBN_num;
					UBN_tb[*pUBN_int].LPN2offset = temp->top;
					if(LUN_tb[*pLUN_int].AL_top->top == PAGE_per_BLOCK - 1){
						temp = LUN_tb[*pLUN_int].AL_top;
						post = temp->next;
						LUN_tb[*pLUN_int].AL_top = post;
						UBN_list_PUSH(UL, END, *pLUN_int, temp->UBN_num);
					}
				}
				PBN[temp_PB].valid[j] = -1;
				PBN[temp_PB].PPN2LPN[j] = -1;
			}
			FB_top++;
			FB_stack[FB_top] = temp_PB;
			inter.erase++;
			erasecnt++;
			cl_info[*pLUN_int].interGC.erase++;
		}
	}



	free(UBN_val);
	on_gc_unit = -1;
	//inter_GC operation
}

int compare_int(UBN_valid_info *arg1, UBN_valid_info *arg2 ){
	if(arg1->UBN_invalid == arg2->UBN_invalid )
		return 0;
	if(arg1->UBN_invalid > arg2->UBN_invalid)
		return -1;
	else
		return 1;
}

void freeing(){

	int i;

	for(i=0; i< UNIT_per_FLASH; i++){
		free(LUN_tb[i].UBN);
	}
	for(i=0; i< BLOCK_per_FLASH; i++){
		free(PBN[i].PPN2LPN);
		free(PBN[i].valid);
	}
	free(PBN);
	free(LUN_tb);
	free(UBN_tb);
	free(FB_stack);
}