#include "main.h"
#include "dftl.h"

extern PBN_MAP *PBN;

partial_cnt Caching_overhead;
partial_cnt GC_overhead;
partial_cnt Caching_inGC_overhead;
partial_cnt map_GC_overhead;


void dftl(char *addr){

	FILE *fp;
	fp = fopen(addr, "r");

	assert(fp);

	dftl_initializing();
	data_unlogging(fp);
	//fclose(fp);
}

void dftl_initializing(){

	int i;
	int j;

	readcnt = 0;
	writecnt = 0;
	erasecnt = 0;

	inGC = 0;

	memset(&IO, 0x00, sizeof(partial_cnt));
	memset(&Caching_overhead, 0x00, sizeof(partial_cnt));
	memset(&GC_overhead, 0x00, sizeof(partial_cnt));
	memset(&Caching_inGC_overhead, 0x00, sizeof(partial_cnt));
	memset(&map_GC_overhead, 0x00, sizeof(partial_cnt));

	CMT_link_front = NULL;
	CMT_cnt = 0;

	current_PB = -1;
	request_cnt = 0;

	CMT_hit_cnt = 0;
	CMT_miss_cnt = 0;

	shortcut_gc_cnt = 0;

	SingleRequestWrite = 0;
	SingleRequestRead = 0;

	SingleRequestCMTWrite = 0;
	SingleRequestCMTRead = 0;

	SingleRequestGCWrite = 0;
	SingleRequestGCRead =0;
	SingleRequestGCErase = 0;
	/*
	FLASH_SECTOR = GB / SECTOR_SIZE * FLASH_SIZE;
	FLASH_PAGE = FLASH_SECTOR / SECTOR_per_PAGE;
	*/
	BLOCK_per_FLASH = LOGICAL_FLASH_BLOCK + OVERPROVISIONING_FLASH_BLOCK;
	FLASH_PAGE = PAGE_per_BLOCK * BLOCK_per_FLASH;
	FLASH_SECTOR = FLASH_PAGE * SECTOR_per_PAGE;

	//BLOCK_per_FLASH = FLASH_SECTOR / (SECTOR_per_PAGE * PAGE_per_BLOCK);
	PAGE_per_FLASH = BLOCK_per_FLASH * PAGE_per_BLOCK;

	MAP_SIZE = _log2(PAGE_per_FLASH);
	if(MAP_SIZE % 8 == 0)
		MAP_SIZE = MAP_SIZE/8;
	else
		MAP_SIZE = MAP_SIZE/8 +1;
	
	if(PAGE_SIZE/MAP_SIZE == 0)
		MAP_ON_PAGE = PAGE_SIZE / MAP_SIZE;
	else
		MAP_ON_PAGE = PAGE_SIZE/ MAP_SIZE +1;

	FLASH_MTB_PAGE = PAGE_per_FLASH/MAP_ON_PAGE;

	if(FLASH_MTB_PAGE%PAGE_per_BLOCK == 0){
		
		FLASH_MTB_BLOCK = FLASH_MTB_PAGE / PAGE_per_BLOCK;
	}
	else 
		FLASH_MTB_BLOCK = FLASH_MTB_PAGE/PAGE_per_BLOCK + 1;

	MAX_MAP_PB = FLASH_MTB_BLOCK * 4;

	LOGICAL_FLASH_BLOCK = LOGICAL_FLASH_BLOCK -
		(FLASH_MTB_PAGE % PAGE_per_BLOCK ? (FLASH_MTB_PAGE / PAGE_per_BLOCK + 1) : (FLASH_MTB_PAGE / PAGE_per_BLOCK));
	LOGICAL_FLASH_PAGE = LOGICAL_FLASH_BLOCK * PAGE_per_BLOCK;

	//LOGICAL_FLASH_PAGE = (FLASH_PAGE - FLASH_MTB_PAGE)*97/100;

	FB_stack = (int *)malloc(sizeof(int)*BLOCK_per_FLASH);
	FB_top = -1;

	MAP_FB_stack = (int *)malloc(sizeof(int)*MAX_MAP_PB);
	MAP_FB_top = -1;
	
	PBN = (PBN_MAP *)malloc(sizeof(PBN_MAP) * BLOCK_per_FLASH);
	LPN2PPN = (PAGE_MAP *)malloc(sizeof(PAGE_MAP) * PAGE_per_FLASH);
	PB_info = (PB_INFO *)malloc(sizeof(PB_INFO) * BLOCK_per_FLASH);


	for(i=0; i<PAGE_per_FLASH; i++){
		LPN2PPN[i].offset = -1;
		LPN2PPN[i].Phy_blk = -1;
	}

	for(i=0; i<BLOCK_per_FLASH; i++){

		FB_stack[i] = i;
		FB_top++;
		PB_info[i].valid_info = 0;
		PB_info[i].invalid_info = 0;
		PB_info[i].top = -1;
		PB_info[i].map_flag = 0;
		PBN[i].PPN2LPN = (int *)malloc(sizeof(int) * PAGE_per_BLOCK);
		PBN[i].valid = (int *)malloc(sizeof(int) * PAGE_per_BLOCK);
		for(j=0; j<PAGE_per_BLOCK; j++){
			PBN[i].PPN2LPN[j] = -1;
			PBN[i].valid[j] = -1;
		}
	}

	for(i=0; i< MAX_MAP_PB; i++){
		MAP_FB_stack[i] = request_FB();
		PB_info[MAP_FB_stack[i]].map_flag = 1;
		MAP_FB_top++;
	}
	printf("\ninitialize end\n");
	
}

void flash_dftl_write(int start, int cnt){
	
	int i;
	int *pLPN_int;
	int LPN_int;
	int FB_receiver = 0;

	SinglePageWrite = 0;
	SinglePageRead = 0;

	SinglePageCMTWrite = 0;
	SinglePageCMTRead = 0;

	SinglePageGCWrite = 0;
	SinglePageGCRead =0;
	SinglePageGCErase = 0;

	pLPN_int = &LPN_int;

	for(i=0; i< cnt; i++){

		request_count_for_IO_dbg++;

		CMT_function(start + i, pLPN_int);

		if((current_PB == -1)||(PB_info[current_PB].top == PAGE_per_BLOCK - 1)){
			FB_receiver = request_FB();
		//FB_receiver로 return되는 request_FB()의 값이 SHOULD_GC(-1)일 경우, GC를 수행해야한다.
			if(FB_receiver == SHOULD_GC)
				DFTL_GC();
			else
				current_PB = FB_receiver;
		}
		if(LPN2PPN[*pLPN_int].offset == -1){
			//아직 해당하는 논리적 주소에 한번도 쓰인적이 없을때
			LPN2PPN[*pLPN_int].Phy_blk = current_PB;
			PB_info[current_PB].top++;
			LPN2PPN[*pLPN_int].offset = PB_info[current_PB].top;
			PB_info[current_PB].valid_info++;
			PBN[current_PB].PPN2LPN[PB_info[current_PB].top] = *pLPN_int;
			PBN[current_PB].valid[PB_info[current_PB].top] = 1;
			writecnt++;
			IO.write++;
			SingleRequestWrite++;
			SinglePageWrite++;

		}
		else{
			//해당하는 논리적 주소에 대한 물리적 영역이 존재. 즉, 데이터가 쓰인적이 있을때
			PBN[LPN2PPN[*pLPN_int].Phy_blk].valid[LPN2PPN[*pLPN_int].offset] = 0;
			PB_info[LPN2PPN[*pLPN_int].Phy_blk].invalid_info++;
			PB_info[LPN2PPN[*pLPN_int].Phy_blk].valid_info--;
			LPN2PPN[*pLPN_int].Phy_blk = current_PB;
			PB_info[current_PB].top++;
			LPN2PPN[*pLPN_int].offset = PB_info[current_PB].top;
			PB_info[current_PB].valid_info++;
			PBN[current_PB].PPN2LPN[PB_info[current_PB].top] = *pLPN_int;
			PBN[current_PB].valid[PB_info[current_PB].top] = 1;
			writecnt++;
			IO.write++;
			SingleRequestWrite++;
			SinglePageWrite++;
		}
#if 0
		if(initializing_flag == FALSE){
			fprintf(SinglePageAnalysisptr, "%d, %d, %d, %d\n", SinglePageWrite * 200 + SinglePageRead*25, SinglePageCMTWrite * 200 + SinglePageCMTRead * 25, SinglePageGCWrite * 200 + SinglePageGCRead * 25 + SinglePageGCErase * 1500, 
				(SinglePageWrite + SinglePageCMTWrite + SinglePageGCWrite)*200 + (SinglePageRead + SinglePageCMTRead + SinglePageGCRead)*25 + SinglePageGCErase * 1500);	
		}
#endif		
		SinglePageWrite = 0;
		SinglePageRead = 0;

		SinglePageCMTWrite = 0;
		SinglePageCMTRead = 0;

		SinglePageGCWrite = 0;
		SinglePageGCRead =0;
		SinglePageGCErase = 0;
	}
	

}

void flash_dftl_read(int start, int cnt){

	int i;
	int *pLPN_int;
	int LPN_int;

	pLPN_int = &LPN_int;

	for(i=0; i<cnt; i++){
		CMT_function(start + i, pLPN_int);
		readcnt++;
		IO.read++;
		SingleRequestRead++;
		SinglePageRead++;

	}

}

void flash_dftl_discard(int start, int cnt){

	int i;
	int *pLPN_int;
	int LPN_int;
	int FB_receiver = 0;

	pLPN_int = &LPN_int;

	for (i = 0; i < cnt; i++){

		CMT_function(start + i, pLPN_int);
		if (LPN2PPN[*pLPN_int].offset == -1){
			return;
		}
		else{
			//해당하는 논리적 주소에 대한 물리적 영역이 존재. 즉, 데이터가 쓰인적이 있을때
			PBN[LPN2PPN[*pLPN_int].Phy_blk].valid[LPN2PPN[*pLPN_int].offset] = 0;
			PB_info[LPN2PPN[*pLPN_int].Phy_blk].invalid_info++;
			PB_info[LPN2PPN[*pLPN_int].Phy_blk].valid_info--;
			LPN2PPN[*pLPN_int].offset = -1;
		}
	}
}

int request_MAP_FB(){

	int temp;
	if(MAP_FB_top < MAP_FB_LIMIT)
		printf("error");
	if(MAP_FB_top <= MAP_FB_LIMIT){
		return SHOULD_GC;
		if(MAP_FB_top == MAP_FB_LIMIT)
			printf("ERROR!! inter GC ERROR occur");
	}
	temp = MAP_FB_top;
	MAP_FB_top--;
	return MAP_FB_stack[temp];
}

void flash_dftl_map_write(int start){

	int FB_receiver = 0;
	if((current_MAP_PB == -1) || (PB_info[current_MAP_PB].top == PAGE_per_BLOCK -1)){
		FB_receiver = request_MAP_FB();
		//FB_receiver로 return되는 request_FB()의 값이 SHOULD_GC(-1)일 경우, GC를 수행해야한다.
		if(FB_receiver == SHOULD_GC){
			DFTL_map_GC();
		}
		else{
			current_MAP_PB = FB_receiver;
		}
	}
	if(LPN2PPN[start].offset == -1){
		//아직 해당하는 논리적 주소에 한번도 쓰인적이 없을때
		LPN2PPN[start].Phy_blk = current_MAP_PB;
		PB_info[current_MAP_PB].top++;
		LPN2PPN[start].offset = PB_info[current_MAP_PB].top;
		PB_info[current_MAP_PB].valid_info++;
		PBN[current_MAP_PB].PPN2LPN[PB_info[current_MAP_PB].top] = start;
		PBN[current_MAP_PB].valid[PB_info[current_MAP_PB].top] = 1;
		writecnt++;
		Caching_overhead.write++;
		if (inGC > 0){
			Caching_inGC_overhead.write++;
		}
		SingleRequestCMTWrite++;
		SinglePageCMTWrite++;
	}
	else{
		//해당하는 논리적 주소에 대한 물리적 영역이 존재. 즉, 데이터가 쓰인적이 있을때
		PBN[LPN2PPN[start].Phy_blk].valid[LPN2PPN[start].offset] = 0;
		PB_info[LPN2PPN[start].Phy_blk].invalid_info++;
		PB_info[LPN2PPN[start].Phy_blk].valid_info--;
		LPN2PPN[start].Phy_blk = current_MAP_PB;
		PB_info[current_MAP_PB].top++;
		LPN2PPN[start].offset = PB_info[current_MAP_PB].top;
		PB_info[current_MAP_PB].valid_info++;
		PBN[current_MAP_PB].PPN2LPN[PB_info[current_MAP_PB].top] = start;
		PBN[current_MAP_PB].valid[PB_info[current_MAP_PB].top] = 1;
		writecnt++;
		Caching_overhead.write++;
		if (inGC > 0){
			Caching_inGC_overhead.write++;
		}
		SingleRequestCMTWrite++;
		SinglePageCMTWrite++;
	}

}

void flash_dftl_map_read(int start){

	readcnt++;
	if (inGC > 0){
		Caching_inGC_overhead.read++;
	}
	Caching_overhead.read++;
	SingleRequestCMTRead++;
	SinglePageCMTRead++;

}
//page based cmt caching
#if 1
void CMT_function(int LPN, int *LPN_int){
	
	list_ptr temp;
	list_ptr post = NULL;
	int VPN_of_LPN;
	short int write_flag = 0;
	int temp1;

	//_CrtMemDumpAllObjectsSince(0);
	temp = CMT_link_front;
	VPN_of_LPN = LOGICAL_FLASH_PAGE + LPN / MAP_ON_PAGE;
	/*
	if(LPN > LOGICAL_FLASH_PAGE){
		*LPN_int = LPN;
	}*/
	while(1){
		if((temp == NULL) || (temp->next==NULL)){
			CMT_miss_cnt++;
			if(((CMT_cnt+1)*PAGE_SIZE)>CMT_SIZE){
				temp1 = temp->data;
				write_flag = 1;
				post->next = NULL;
				flash_dftl_map_read(VPN_of_LPN);
				temp->next = CMT_link_front;
				CMT_link_front = temp;
				temp->data = VPN_of_LPN;
				*LPN_int = LPN;
				break;
			}
			else{
				if(temp == NULL){
					temp = create_node();
					CMT_cnt++;
					flash_dftl_map_read(VPN_of_LPN);
					CMT_link_front = temp;
					temp->data = VPN_of_LPN;
					*LPN_int = LPN;
					break;
				}
				else{
					temp = create_node();
					CMT_cnt++;
					flash_dftl_map_read(VPN_of_LPN);
					temp->next = CMT_link_front;
					CMT_link_front = temp;
					temp->data = VPN_of_LPN;
					*LPN_int = LPN;
					break;
				}
			}
		}
		else{
			if(temp->data == VPN_of_LPN){
				CMT_hit_cnt++;
				if(temp == CMT_link_front){
					*LPN_int = LPN;
					break;
				}
				else{
					post->next = temp->next;
					temp->next = CMT_link_front;
					CMT_link_front = temp;
					*LPN_int = LPN;
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
		flash_dftl_map_write(temp1);
		write_flag = 0;
	}
	
}
#endif
#if 0
//entry-based CMT caching
void CMT_function(int LPN, int *LPN_int){
	
	list_ptr temp;
	list_ptr post;
	int VPN_of_LPN;
	short int write_flag = 0;
	int temp1;

	//_CrtMemDumpAllObjectsSince(0);
	temp = CMT_link_front;
	VPN_of_LPN = LOGICAL_FLASH_PAGE + LPN / MAP_ON_PAGE;
	/*
	if(LPN > LOGICAL_FLASH_PAGE){
		*LPN_int = LPN;
	}*/
	while(1){
		if((temp == NULL) || (temp->next==NULL)){
			CMT_miss_cnt++;
			if((CMT_cnt+1) > CMT_SIZE / PAGE_SIZE){
				temp1 = temp->data;
				write_flag = 1;
				post->next = NULL;
				flash_dftl_map_read(VPN_of_LPN);
				temp->next = CMT_link_front;
				CMT_link_front = temp;
				temp->data = LPN;
				*LPN_int = LPN;
				break;
			}
			else{
				if(temp == NULL){
					temp = create_node();
					CMT_cnt++;
					flash_dftl_map_read(VPN_of_LPN);
					CMT_link_front = temp;
					temp->data = LPN;
					*LPN_int = LPN;
					break;
				}
				else{
					temp = create_node();
					CMT_cnt++;
					flash_dftl_map_read(VPN_of_LPN);
					temp->next = CMT_link_front;
					CMT_link_front = temp;
					temp->data = LPN;
					*LPN_int = LPN;
					break;
				}
			}
		}
		else{
			if(temp->data == LPN){
				CMT_hit_cnt++;
				if(temp == CMT_link_front){
					*LPN_int = LPN;
					break;
				}
				else{
					post->next = temp->next;
					temp->next = CMT_link_front;
					CMT_link_front = temp;
					*LPN_int = LPN;
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
		flash_dftl_map_write(LOGICAL_FLASH_PAGE + temp1 / MAP_ON_PAGE);
		write_flag = 0;
	}
	
}
#endif

void DFTL_GC(){

	int i;
	int max_invalid_blk = 0;
	int victim_blk;
	int target_FB;
	int *pLPN_int;
	int LPN_int;
	int first_flag = 1;

	inGC++;

	pLPN_int = &LPN_int;

	for(i=0; i<BLOCK_per_FLASH; i++){
		if(PB_info[i].map_flag == 0){
			if(first_flag == 1){
				max_invalid_blk = i;
				first_flag = 0;
			}
			else if(PB_info[max_invalid_blk].invalid_info < PB_info[i].invalid_info)
				max_invalid_blk = i;
		}
	}

	victim_blk = max_invalid_blk;
	target_FB = FB_stack[FB_top];
	FB_top--;

	if(PB_info[victim_blk].valid_info > 0){
		for(i=0; i<PAGE_per_BLOCK; i++){
			/*
			readcnt++;
			GC_overhead.read++;
			SingleRequestGCRead++;
			SinglePageGCRead++;
			*/
			if(PBN[victim_blk].valid[i] == 1){
				//page valid map이 있다고 가정할때 read 역시 valid 할때만 수행된다.
				readcnt++;
				GC_overhead.read++;
				SingleRequestGCRead++;
				SinglePageGCRead++;
				writecnt++;
				GC_overhead.write++;
				SingleRequestGCWrite++;
				SinglePageGCWrite++;
				CMT_function(PBN[victim_blk].PPN2LPN[i], pLPN_int);
				PB_info[target_FB].top++;
				PBN[target_FB].PPN2LPN[PB_info[target_FB].top] = PBN[victim_blk].PPN2LPN[i];
				PBN[target_FB].valid[PB_info[target_FB].top] = 1;
				LPN2PPN[PBN[victim_blk].PPN2LPN[i]].Phy_blk = target_FB;
				LPN2PPN[PBN[victim_blk].PPN2LPN[i]].offset = PB_info[target_FB].top;
				PB_info[target_FB].valid_info++;
			}
		}
	}
	for(i=0; i<PAGE_per_BLOCK; i++){
		PBN[victim_blk].PPN2LPN[i] = -1;
		PBN[victim_blk].valid[i] = -1;
	}
	PB_info[victim_blk].invalid_info = 0;
	PB_info[victim_blk].top = -1;
	PB_info[victim_blk].valid_info = 0;

	FB_top++;
	FB_stack[FB_top] = victim_blk;
	current_PB = target_FB;
	erasecnt++;
	GC_overhead.erase++;
	SingleRequestGCErase++;
	SinglePageGCErase++;

	inGC--;
}

void DFTL_map_GC(){
	
	int i;
	int max_invalid_blk = 0;
	int first_flag = 1;
	int victim_blk;
	int target_FB;

	for(i=0; i<BLOCK_per_FLASH; i++){
		if(PB_info[i].map_flag == 1){
			if(first_flag == 1){
				max_invalid_blk = i;
				first_flag = 0;
			}
			else if(PB_info[max_invalid_blk].invalid_info < PB_info[i].invalid_info)
				max_invalid_blk = i;
		}
	}
	victim_blk = max_invalid_blk;
	target_FB = MAP_FB_stack[MAP_FB_top];
	MAP_FB_top--;

	if(PB_info[victim_blk].valid_info > 0){
		for(i=0; i<PAGE_per_BLOCK; i++){
			readcnt++;
			GC_overhead.read++;
			SingleRequestGCRead++;
			SinglePageGCRead++;
			if(PBN[victim_blk].valid[i] == 1){
				writecnt++;
				GC_overhead.write++;
				SingleRequestGCWrite++;
				SinglePageGCWrite++;
				PB_info[target_FB].top++;
				//error detection
				PBN[target_FB].PPN2LPN[PB_info[target_FB].top] = PBN[victim_blk].PPN2LPN[i];
				PBN[target_FB].valid[PB_info[target_FB].top] = 1;
				LPN2PPN[PBN[victim_blk].PPN2LPN[i]].Phy_blk = target_FB;
				LPN2PPN[PBN[victim_blk].PPN2LPN[i]].offset = PB_info[target_FB].top;
				PB_info[target_FB].valid_info++;
			}
		}
	}
	for(i=0; i<PAGE_per_BLOCK; i++){
		PBN[victim_blk].PPN2LPN[i] = -1;
		PBN[victim_blk].valid[i] = -1;
	}
	PB_info[victim_blk].invalid_info = 0;
	PB_info[victim_blk].top = -1;
	PB_info[victim_blk].valid_info = 0;

	MAP_FB_top++;
	MAP_FB_stack[MAP_FB_top] = victim_blk;
	current_MAP_PB = target_FB;
	erasecnt++;
	GC_overhead.erase++;
	SingleRequestGCErase++;
	SinglePageGCErase++;

}

void DFTL_freeing(){

	int i;
	
	free(FB_stack);
	free(PB_info);
	free(LPN2PPN);
	
	for(i=0; i < BLOCK_per_FLASH; i++){
		free(PBN[i].PPN2LPN);
		free(PBN[i].valid);
	}
	free(PBN);

}