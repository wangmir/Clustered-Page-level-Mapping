#include "main.h"



void main(int argc, char *argv[]){

	FILE *fp;
	char *log_addr;

	log_addr = argv[2];
	trace_addr = argv[2];
	IO_at_top.read=0;
	IO_at_top.write=0;

	request_count_for_IO_dbg = 0;

	print_welcome();

	fp = fopen(log_addr, "rt");
	assert(fp);
	command_setting(argc, argv);
	fclose(fp);
	if(strcmp(argv[1], "ulm")==0){
		printf("\nFTL : Unit Level Mapping\n");
		FTL_FLAG = ULM;
		unit_level_mapping(log_addr);
	}
	else if(strcmp(argv[1], "dftl")==0){
		printf("\nFTL : DFTL\n");
		FTL_FLAG = DFTL;
		dftl(log_addr);
	}
	evaluation(log_addr);
	if(strcmp(argv[1], "ulm") == 0){
		freeing();
	}
	else if(strcmp(argv[1], "dftl") == 0){
		DFTL_freeing();
	}
}

void print_welcome(){

	printf("===============================================\n");
	printf("================FTL simulator==================\n");
	printf("Auther : Kim, Hyukjoong, SKKU, ESLab\n");
	printf("Contact : wangmir@skku.edu\n");
	printf("===============================================\n");

}

void command_setting(int argc, char *argv[]){

	int i;
	int temp;
	SECTOR_per_PAGE = 16;
	PAGE_per_BLOCK = 128;
	BLOCK_per_UNIT = 256;
	CMT_SIZE = 64*KB;
	FLASH_SIZE = 4;
	BLOCK_per_FLASH = 4096;
	UBN_CMT_PCT = 80;
	LUN_CMT_PCT = 20;
	MAX_PB_per_UNIT = 0;
	VALID_WATCH_FLAG = 0;
	TRACE_CUTTER = 0;

	for(i=0;i<argc;i++){

		if(strcmp(argv[i], "-sp")==0){

			i++;
			SECTOR_per_PAGE = atoi(argv[i]);

		}
		if(strcmp(argv[i], "-pb")==0){

			i++;
			PAGE_per_BLOCK = atoi(argv[i]);
		}
		if(strcmp(argv[i], "-bu")==0){
			i++;
			BLOCK_per_UNIT = atoi(argv[i]);
			
		}
		if(strcmp(argv[i], "-cmt")==0){

			i++;
			CMT_SIZE = atoi(argv[i])*KB;
		}
		if(strcmp(argv[i], "-pct")==0){

			i++;
			UBN_CMT_PCT = atoi(argv[i]);
			LUN_CMT_PCT = 100 - UBN_CMT_PCT;
		}
		/*
		if(strcmp(argv[i], "-fs")==0){

			i++;
			FLASH_SIZE = atoi(argv[i]);
		}
		*/
		if (strcmp(argv[i], "-fb") == 0){
			i++;
			LOGICAL_FLASH_BLOCK = atoi(argv[i]);
		} //Modify flash size as the number of block in the flash

		if (strcmp(argv[i], "-op") == 0){
			i++;
			OVERPROVISIONING_FLASH_BLOCK = atoi(argv[i]);
		}

		if(strcmp(argv[i], "-pu")==0){

			i++;
			PB_per_UNIT_ratio = atoi(argv[i]);
			MAX_PB_per_UNIT = BLOCK_per_UNIT + BLOCK_per_UNIT * PB_per_UNIT_ratio / 2;
		}
		if(strcmp(argv[i], "-init")==0){

			initializing_flag = INITIAL_WRITE;
			i++;
			if(strcmp(argv[i], "seq")==0){
				initializing_flag = INITIAL_SEQUENTIAL_WRITE;
			}
			else if(strcmp(argv[i], "rand")==0){
				initializing_flag = INITIAL_RANDOM_WRITE;
			}
			else if (strcmp(argv[i], "sr") == 0){
				initializing_flag = INITIAL_SEQRAND_WRITE;
			}
			else{
				initializing_trace_addr = (char *)malloc(sizeof(argv[i])+5);
				initializing_trace_addr = argv[i];
			}
		}
		if(strcmp(argv[i], "-validwatch") == 0)
			VALID_WATCH_FLAG = 1;
		if(strcmp(argv[i], "-cut") == 0){
			i++;
			TRACE_CUTTER = atoi(argv[i]);
		}

	}

	if(MAX_PB_per_UNIT == 0){
		MAX_PB_per_UNIT = BLOCK_per_UNIT * 2;
	}
	PAGE_SIZE = SECTOR_SIZE * SECTOR_per_PAGE;
	BLOCK_SIZE = PAGE_SIZE * PAGE_per_BLOCK;
	PAGE_per_UNIT = PAGE_per_BLOCK * BLOCK_per_UNIT;

	temp = GB/PAGE_SIZE;
	TRACE_CUTTER = TRACE_CUTTER*temp;

	printf("\nSector per page is %d", SECTOR_per_PAGE);
	printf("\nPage per block is %d", PAGE_per_BLOCK);
	printf("\nBlock per unit is %d", BLOCK_per_UNIT);
	printf("\nCMT size is %d\n", CMT_SIZE);

}


void data_unlogging(FILE *fp){

	int i;
	IOData temp_data;
	int temp;
	FILE *fp1, *fp2;
	char addr[200];
	extern partial_cnt intra;
	extern partial_cnt inter;
	extern partial_cnt LUN_func;
	extern partial_cnt UBN_func;
	extern partial_cnt GC_overhead;
	extern partial_cnt Caching_overhead;
	extern partial_cnt map_GC_overhead;
	extern partial_cnt Caching_inGC_overhead;

	int overhead;

	discard_start = -1;
	
	if(initializing_flag == INITIAL_WRITE){
		fp1 = fopen(initializing_trace_addr, "rt");
		printf("\nInitializing START\n");
		for(i=0;;i++){
			
			if(i%5000 == 1)
				printf("-");

			if (i < LOGICAL_FLASH_PAGE * 20 / 100){
				flash_write(i, 1);
				continue;
			}
			
			if(read_log(fp1, &temp_data)==1){

				//printf("\n%d, %f, %d, %d, %d, %d", temp_data.dwIndex, temp_data.fTime, temp_data.dwDevId, temp_data.bRead, temp_data.dwPageNo, temp_data.dwCount); 
			
				if(temp_data.dwPageNo>LOGICAL_FLASH_PAGE){

					//printf("\nERROR!! :: Required traces page number is too large for this FLASH mem.");
					//printf("\nRequired page number : %d\n", temp_data.dwPageNo);
					continue;
				}

				if(temp_data.bRead == 0){
					flash_write(temp_data.dwPageNo, temp_data.dwCount);
				}
				else if(temp_data.bRead == 1){
					flash_read(temp_data.dwPageNo, temp_data.dwCount);
				}
				else{
					printf("ERROR!! :: Write/Read operator is not found in log file's current data log.");
					exit(1);
				}
			}
			else{
				printf("\ninitializing END\n");
				break;
			}
		}
		
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
		UBN_hit_cnt = 0;
		UBN_miss_cnt = 0;
		LUN_hit_cnt = 0;
		LUN_miss_cnt = 0;
		shortcut_inter_cnt = 0;
		shortcut_intra_cnt = 0;
		shortcut_gc_cnt = 0;

		memset(&IO, 0x00, sizeof(partial_cnt));
		memset(&Caching_overhead, 0x00, sizeof(partial_cnt));
		memset(&GC_overhead, 0x00, sizeof(partial_cnt));
		memset(&Caching_inGC_overhead, 0x00, sizeof(partial_cnt));
		memset(&map_GC_overhead, 0x00, sizeof(partial_cnt));

		if (FTL_FLAG == ULM)
			memset(cl_info, 0x00, sizeof(CL_INFO) * UNIT_per_FLASH);

		tot_remain_space = 0;

		request_cnt = 0;
		CMT_hit_cnt = 0;
		CMT_miss_cnt = 0;
		fclose(fp1);
	}
	else if(initializing_flag == INITIAL_SEQUENTIAL_WRITE){
		//플래시 스토리지를 전체적으로 논리적이게 연속적으로 writing
		for(i=0;;i++){
			if(i%5000 == 1)
				printf("-");
			flash_write(i,1);
			if(i==LOGICAL_FLASH_PAGE * 70 / 100)
				break;
		}
		printf("\n sequential initializing END\n");
		
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
		UBN_hit_cnt = 0;
		UBN_miss_cnt = 0;
		LUN_hit_cnt = 0;
		LUN_miss_cnt = 0;
		shortcut_inter_cnt = 0;
		shortcut_intra_cnt = 0;
		shortcut_gc_cnt = 0;

		memset(&IO, 0x00, sizeof(partial_cnt));
		memset(&Caching_overhead, 0x00, sizeof(partial_cnt));
		memset(&GC_overhead, 0x00, sizeof(partial_cnt));
		memset(&Caching_inGC_overhead, 0x00, sizeof(partial_cnt));
		memset(&map_GC_overhead, 0x00, sizeof(partial_cnt));

		if (FTL_FLAG == ULM)
			memset(cl_info, 0x00, sizeof(CL_INFO) * UNIT_per_FLASH);

		tot_remain_space = 0;

		request_cnt = 0;
		CMT_hit_cnt = 0;
		CMT_miss_cnt = 0;
	}

	else if (initializing_flag == INITIAL_SEQRAND_WRITE){
		//플래시 스토리지를 전체적으로 논리적이게 연속적으로 writing
		
		RandomInit(1);
		for (i = 0;; i++){
			if (i % 5000 == 1)
				printf("-");
			if(i < LOGICAL_FLASH_PAGE * 75 / 100)
				flash_write(i, 1);
			else{
				temp = IRandom(0, LOGICAL_FLASH_PAGE * 25 / 100);
				flash_write(temp, 1);
				if (i == PAGE_per_FLASH)
					break;
			}
		}
		printf("\n random initializing END\n");
		
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
		UBN_hit_cnt = 0;
		UBN_miss_cnt = 0;
		LUN_hit_cnt = 0;
		LUN_miss_cnt = 0;
		shortcut_inter_cnt = 0;
		shortcut_intra_cnt = 0;
		shortcut_gc_cnt = 0;

		memset(&IO, 0x00, sizeof(partial_cnt));
		memset(&Caching_overhead, 0x00, sizeof(partial_cnt));
		memset(&GC_overhead, 0x00, sizeof(partial_cnt));
		memset(&Caching_inGC_overhead, 0x00, sizeof(partial_cnt));
		memset(&map_GC_overhead, 0x00, sizeof(partial_cnt));

		if (FTL_FLAG == ULM)
			memset(cl_info, 0x00, sizeof(CL_INFO) * UNIT_per_FLASH);

		tot_remain_space = 0;

		request_cnt = 0;
		CMT_hit_cnt = 0;
		CMT_miss_cnt = 0;
	}
	else if (initializing_flag == INITIAL_RANDOM_WRITE){
		//플래시 스토리지를 전체적으로 논리적이게 연속적으로 writing
		RandomInit(1);
		for (i = 0;; i++){
			if (i % 5000 == 1)
				printf("-");
			temp = IRandom(0, LOGICAL_FLASH_PAGE * 70 / 100);
			flash_write(temp, 1);
			if (i == PAGE_per_FLASH)
				break;
		}
		printf("\n random initializing END\n");

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
		LUN_func.write = 0;
		UBN_func.erase = 0;
		UBN_func.read = 0;
		UBN_func.write = 0;
		UBN_hit_cnt = 0;
		UBN_miss_cnt = 0;
		LUN_hit_cnt = 0;
		LUN_miss_cnt = 0;
		shortcut_inter_cnt = 0;
		shortcut_intra_cnt = 0;
		shortcut_gc_cnt = 0;

		memset(&IO, 0x00, sizeof(partial_cnt));
		memset(&Caching_overhead, 0x00, sizeof(partial_cnt));
		memset(&GC_overhead, 0x00, sizeof(partial_cnt));
		memset(&Caching_inGC_overhead, 0x00, sizeof(partial_cnt));
		memset(&map_GC_overhead, 0x00, sizeof(partial_cnt));

		if (FTL_FLAG == ULM)
			memset(cl_info, 0x00, sizeof(CL_INFO) * UNIT_per_FLASH);

		tot_remain_space = 0;

		request_cnt = 0;
		CMT_hit_cnt = 0;
		CMT_miss_cnt = 0;
	}
	
	//initializing_flag = FALSE;
#if 1
	if (FTL_FLAG == DFTL)
		sprintf(addr, "%s_resp_dftl", trace_addr);
	else
		sprintf(addr, "%s_resp_ulm", trace_addr);
	fp2 = fopen(addr, "w+");
#endif
	for(i=0;;i++){
		
		if(i%5000 == 1)
			printf("-");
			
		if(read_log(fp, &temp_data)==1){

			//printf("\n%d, %f, %d, %d, %d, %d", temp_data.dwIndex, temp_data.fTime, temp_data.dwDevId, temp_data.bRead, temp_data.dwPageNo, temp_data.dwCount); 
#if 1
			if (i != 0){
				response.write = writecnt - response.write;
				response.read = readcnt - response.read;
				response.erase = erasecnt - response.erase;

				response_IO.write = IO.write - response_IO.write;
				response_IO.read = IO.read - response_IO.read;

				if (FTL_FLAG == DFTL){
					response_GC.write = GC_overhead.write - response_GC.write;
					response_GC.read = GC_overhead.read - response_GC.read;
					response_GC.erase = GC_overhead.erase - response_GC.erase;

					response_MAP.write = Caching_overhead.write - response_MAP.write;
					response_MAP.read = Caching_overhead.read - response_MAP.read;

					response_MAP_inGC.write = Caching_inGC_overhead.write - response_MAP_inGC.write;
					response_MAP_inGC.read = Caching_inGC_overhead.read - response_MAP_inGC.read; 
				}
				else if (FTL_FLAG == ULM){
					response_GC.write = intra.write + inter.write - response_GC.write;
					response_GC.read = intra.read + inter.read - response_GC.read;
					response_GC.erase = intra.erase + inter.erase - response_GC.erase;

					response_MAP.write = LUN_func.write + UBN_func.write - response_MAP.write;
					response_MAP.read = LUN_func.read + UBN_func.read - response_MAP.read;
				}

				//fprintf(fp2, "%d, %d, %s, %d, %d, %d\n", temp_data.dwSectNo, temp_data.dwSectCount, temp_data.bRead?"Read":"Write", response.write, response.read, response.erase);
				fprintf(fp2, "%d, %d, %d, %d, %d\n", 
					response.write * 800 + response.read * 60 + response.erase * 1600, response_IO.write * 800 + response_IO.read * 60, response_GC.write * 800 + response_GC.read * 60 + response_GC.erase * 1600, response_MAP.write * 800 + response_MAP.read * 60
					, response_MAP_inGC.write * 800 + response_MAP_inGC.read * 60);
			}
			memset(&response, 0x00, sizeof(partial_cnt));
			memset(&response_GC, 0x00, sizeof(partial_cnt));
			memset(&response_IO, 0x00, sizeof(partial_cnt));
			memset(&response_MAP, 0x00, sizeof(partial_cnt));
			memset(&response_MAP_inGC, 0x00, sizeof(partial_cnt));

			response.write = writecnt;
			response.read = readcnt;
			response.erase = erasecnt;

			response_IO.write = IO.write;
			response_IO.read = IO.read;
			if (FTL_FLAG == DFTL){
				response_GC.write = GC_overhead.write;
				response_GC.read = GC_overhead.read;
				response_GC.erase = GC_overhead.erase;

				response_MAP.write = Caching_overhead.write;
				response_MAP.read = Caching_overhead.read;

				response_MAP_inGC.write = Caching_inGC_overhead.write;
				response_MAP_inGC.read = Caching_inGC_overhead.read;
			}
			else if(FTL_FLAG == ULM){
				response_GC.write = intra.write + inter.write;
				response_GC.read = intra.read + inter.read;
				response_GC.erase = intra.erase + inter.erase;

				response_MAP.write = LUN_func.write + UBN_func.write;
				response_MAP.read = LUN_func.read + UBN_func.read;
			}
#endif

			if(temp_data.dwPageNo>LOGICAL_FLASH_PAGE){

				printf("\nERROR!! :: Required traces page number is too large for this FLASH mem.");
				printf("\nRequired page number : %d\n", temp_data.dwPageNo);
				continue;

			}

			if(temp_data.bRead == 0){
				IO_at_top.write = IO_at_top.write + temp_data.dwCount;
				if (TRACE_CUTTER != 0){
					if (IO_at_top.write > TRACE_CUTTER)
						break;
				}
				
				if (temp_data.dwSectNo % SECTOR_per_PAGE == 0){
					if ((temp_data.dwSectNo + temp_data.dwSectCount) % SECTOR_per_PAGE == 0){
						//all aligned
						flash_write(temp_data.dwPageNo, temp_data.dwCount);
					}

					else{
						//시작 주소는 aligned, but count는 not aligned
						flash_read((temp_data.dwSectNo + temp_data.dwSectCount) / SECTOR_per_PAGE, 1);
						IO.read--; //실제 IO read가 아니라 read copy write를 위한 read기 때문에
						flash_write(temp_data.dwPageNo, temp_data.dwCount);
					}
				}
				else {
					flash_read(temp_data.dwPageNo, 1);
					IO.read--;
					if ((temp_data.dwSectNo + temp_data.dwSectCount) % SECTOR_per_PAGE == 0){
						//시작 주소는 not aligned, count는 ali
						flash_write(temp_data.dwPageNo, temp_data.dwCount);
					}
					else{
						flash_read((temp_data.dwSectNo + temp_data.dwSectCount) / SECTOR_per_PAGE, 1);
						IO.read--;
						flash_write(temp_data.dwPageNo, temp_data.dwCount);
					}


				}

			}
			else if(temp_data.bRead == 1){
				
				if(temp_data.dwCount < 0)
					printf("!!");
				//read operation을 고려하지 않는 실험
				IO_at_top.read = IO_at_top.read + temp_data.dwCount;
				flash_read(temp_data.dwPageNo, temp_data.dwCount);


			}
			else if (temp_data.bRead == 2){

				int start;
				start = (temp_data.dwSectNo % SECTOR_per_PAGE == 0) ? temp_data.dwSectNo / SECTOR_per_PAGE : temp_data.dwSectNo / SECTOR_per_PAGE + 1;

				flash_discard_start(start, 0);

			}

			else if (temp_data.bRead == 3){
				int end;
				end = (temp_data.dwSectNo % SECTOR_per_PAGE == 0) ? temp_data.dwSectNo / SECTOR_per_PAGE : temp_data.dwSectNo / SECTOR_per_PAGE - 1;
				flash_discard_end(end, 0);
			}
			else{
				printf("ERROR!! :: Write/Read operator is not found in log file's current data log.");
				exit(1);
			}
		}
		else{
			printf("\nData log read&write is completed.\n");
			//fclose(fp2);

			break;
		}

	}
	//fclose(fp2);

}

short int read_log(FILE *fp, void* ptrData){

	double fDuration;
	char psIO[20];
	int temp1, temp2;
	IOData *pData;

	pData = (IOData *)ptrData;

	pData->fTime = 0;

	if(feof(fp)==0){

		fscanf(fp, "%d %f %lf %d %s %d %d", &pData->dwIndex, &pData->fTime,
							&fDuration, &pData->dwDevId, psIO, &temp1, &temp2);

		pData->dwSectNo = temp1;
		pData->dwSectCount = temp2;

		// need recalculation of page no.
		if(temp1%SECTOR_per_PAGE == 0)
			pData->dwPageNo=temp1/SECTOR_per_PAGE;
		else
			pData->dwPageNo= temp1/SECTOR_per_PAGE;

		if ((temp1 + temp2) % SECTOR_per_PAGE == 0)
			pData->dwCount = (temp1 + temp2) / SECTOR_per_PAGE - temp1 / SECTOR_per_PAGE;
		else
			pData->dwCount = (temp1 + temp2) / SECTOR_per_PAGE + 1 - temp1 / SECTOR_per_PAGE;
		/*
		if(temp2%SECTOR_per_PAGE == 0)
			pData->dwCount = temp2/SECTOR_per_PAGE;
		else
			pData->dwCount = temp2/SECTOR_per_PAGE + 1;
			*/

		if (strcmp(psIO, "Write") == 0 || strcmp(psIO, "WriteSync") == 0 || strcmp(psIO, "25") == 0)
		{
			pData->bRead = 0;
		}
		else if(strcmp(psIO, "Read") == 0 || strcmp(psIO, "18") == 0)
		{
			pData->bRead = 1;
		}
		else if (strcmp(psIO, "35") == 0)
		{
			pData->bRead = 2; 
		}
		else if (strcmp(psIO, "36") == 0){
			pData->bRead = 3;
		}

		else
		{
			printf("\nfail to check R/W \n");
			return 0;
		}
		return 1;
	}
	else{

		printf("\nData reading is end.\n");
		return 0;
	}
}

void flash_write(int start, int cnt){

	SingleRequestWrite = 0;
	SingleRequestRead = 0;

	SingleRequestCMTWrite = 0;
	SingleRequestCMTRead = 0;

	SingleRequestGCWrite = 0;
	SingleRequestGCRead =0;
	SingleRequestGCErase = 0;

	if(FTL_FLAG == ULM)
		flash_ulm_write(start, cnt);
	else if(FTL_FLAG == DFTL)
		flash_dftl_write(start, cnt);

	

}

void flash_read(int start, int cnt){

	SingleRequestWrite = 0;
	SingleRequestRead = 0;

	SingleRequestCMTWrite = 0;
	SingleRequestCMTRead = 0;

	SingleRequestGCWrite = 0;
	SingleRequestGCRead =0;
	SingleRequestGCErase = 0;

	if(FTL_FLAG == 1)
		flash_ulm_read(start, cnt);
	else if(FTL_FLAG == 2)
		flash_dftl_read(start, cnt);

	

}

void flash_discard_start(int start, int cnt){

	discard_start = start;


}

void flash_discard_end(int start, int cnt){

	int count;

	if (discard_start < 0)
		return;
	if (discard_start > start){
		discard_start = -1;
		return;
	}

	count = start - discard_start + 1;
	if (FTL_FLAG == 1)
		flash_ulm_discard(discard_start, count);
	else if (FTL_FLAG == 2)
		flash_dftl_discard(discard_start, count);
}

/*
void Count(int operation, int Layer){

	int op_flag;
	

	switch(operation){
	case READ: 
		op_flag = READ;
	case WRITE:
		op_flag = WRITE;
	case ERASE:
		op_flag = ERASE;
	}

	switch(Layer){
	case IO:
		if(op_flag == READ){

		}
	case CMT:
		if(op_flag == WRITE){

		}
	case GC:
		if(op_flag == ERASE){

		}
		
	}
}
*/
int request_FB(){

	int temp;
	//if(FB_top < FB_LIMIT)
		//printf("error");
	if(FB_top <= FB_LIMIT){
		if(FTL_FLAG == ULM)
			inter_GC();
		else if(FTL_FLAG == DFTL)
			return SHOULD_GC;
		//if(FB_top == FB_LIMIT)
			//printf("ERROR!! inter GC ERROR occur");

	}
	temp = FB_top;
	FB_top--;
	return FB_stack[temp];
}

list_ptr create_node(){

	list_ptr temp;
	temp = (list *)malloc(sizeof(list));
	temp->next = NULL;
	return temp;
}

int _log2(int base){

	float temp1, temp2, temp3;
	int temp4;

	temp1 = log10((double)base);
	temp2 = log10(2.0);
	temp3 = temp1/temp2;
	temp4 = (int)temp3;
	return temp3;
}

void evaluation(char *addr){

	char file_name[256];
	const float read_rate = 25;
	const float write_rate = 200;
	const float erase_rate = 1500;

	FILE *fp;
	FILE *cl_info_fpout = NULL;
	int iter;
	

	double total_time;
	double gc_time;
	double intra_gc_time;
	double inter_gc_time;
	double io_time;
	double io_at_top_time;
	double map_rw_time;
	float extra_PB;
	char *initial_string;

	extern partial_cnt intra;
	extern partial_cnt inter;
	extern partial_cnt LUN_func;
	extern partial_cnt UBN_func;
	extern partial_cnt GC_overhead;
	extern partial_cnt Caching_overhead;
	extern partial_cnt map_GC_overhead;
	extern partial_cnt Caching_inGC_overhead;

	total_time = read_rate * readcnt + write_rate * writecnt + erase_rate * erasecnt;
	intra_gc_time = read_rate * intra.read + write_rate * intra.write + erase_rate * intra.erase;
	inter_gc_time = read_rate * inter.read + write_rate * inter.write + erase_rate * inter.erase;
	if(FTL_FLAG == ULM){
		gc_time = intra_gc_time + inter_gc_time;
	}
	else if(FTL_FLAG == DFTL){
		gc_time = read_rate * GC_overhead.read + write_rate * GC_overhead.write + erase_rate * GC_overhead.erase;
	}
	io_time = read_rate*IO.read + write_rate * IO.write;
	io_at_top_time = read_rate * IO_at_top.read + write_rate * IO_at_top.write;
	if(FTL_FLAG == ULM){
		if(LUN_ON_PAGE > 1)
			map_rw_time = read_rate * (LUN_func.read + UBN_func.read) + write_rate * (LUN_func.write  + UBN_func.write);
		else
			map_rw_time = read_rate * (LUN_func.read* LUN_PAGE_no + UBN_func.read) + write_rate * (LUN_func.write * LUN_PAGE_no + UBN_func.write);
	}
	else if(FTL_FLAG == DFTL){
		map_rw_time = read_rate * Caching_overhead.read + write_rate * Caching_overhead.write;
	}

	printf("\n\nBy SAMSUNG NAND Flash chip K9F1G16U0M, total time is %lf seconds.\n", total_time/1000000);

	initial_string = (char *)malloc(sizeof(char) * 30);

	if(initializing_flag == INITIAL_WRITE)
		initial_string = "INITIALIZED";
	else if (initializing_flag == INITIAL_SEQUENTIAL_WRITE)
		initial_string = "SEQ_INITIALIZED";
	else if (initializing_flag == INITIAL_RANDOM_WRITE)
		initial_string = "RAND_INITIALIZED";
	else
		initial_string = "NON_INITIALIZED";
	
	if(VALID_WATCH_FLAG == 1){
		valid_watcher(addr);
	}
#if 1
	/*
	if(FTL_FLAG == ULM)
		sprintf(file_name, "%s_%s- ULM evaluation.txt", addr, initial_string);
	else if(FTL_FLAG == DFTL)
		sprintf(file_name, "%s_%s- DFTL evaluation.txt", addr, initial_string);
		*/
	if(FTL_FLAG == ULM)
		sprintf(file_name, "%s_%s_ULM_eval", addr, initial_string);

	else if(FTL_FLAG == DFTL)
		sprintf(file_name, "%s_%s_DFTL_eval", addr, initial_string);
	//free(initial_string);
#if 0
	fp = fopen(file_name,"r");
	if(fp == NULL){
		fp = fopen(file_name, "a+");
		if(FTL_FLAG == ULM)
			fprintf(fp, "FTL, BLOCK_per_UNIT, FLASH_MTB_PAGE, IO_at_top.write, IO_at_top.read, LUN_func.write, LUN_func.read, UBN_func.write, UBN_func.read, intra.write, intra.read, intra.erase, inter.write, inter.read, inter.erase, UBN_hit_cnt, UBN_miss_cnt, LUN_hit_cnt, LUN_miss_cnt, intra_GC_cnt, inter_GC_cnt, CMT_SIZE, UBN_CMT_PCT, PB_per_UNIT_ratio, intra_gc_time, inter_gc_time, gc_time, io_at_top_time, map_rw_time, total_time\n");
		else if(FTL_FLAG == DFTL)
			fprintf(fp, "FTL, FLASH_MTB_PAGE, IO_at_top.write, IO_at_top.read, Caching_overhead.write, Caching_overhead.read, GC_overhead.write, GC_overhead.read, GC_overhead.erase, CMT_hit_cnt, CMT_miss_cnt, CMT_SIZE, gc_time, io_at_top_time, map_rw_time, total_time\n");
	}
	else{
		fclose(fp);
		fp=fopen(file_name, "a+");
	}
	if(FTL_FLAG == ULM)
		fprintf(fp, "ULM, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %lf, %lf, %lf\n", BLOCK_per_UNIT, FLASH_MTB_PAGE, IO_at_top.write, IO_at_top.read, LUN_func.write, LUN_func.read, UBN_func.write, UBN_func.read, intra.write, intra.read, intra.erase, inter.write, inter.read, inter.erase, UBN_hit_cnt, UBN_miss_cnt, LUN_hit_cnt, LUN_miss_cnt, intra_GC_cnt, inter_GC_cnt, CMT_SIZE, UBN_CMT_PCT, PB_per_UNIT_ratio, intra_gc_time, inter_gc_time, gc_time, io_at_top_time, map_rw_time, total_time);
	else if(FTL_FLAG == DFTL)
		fprintf(fp, "DFTL, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %lf, %lf, %lf, %lf\n", FLASH_MTB_PAGE, IO_at_top.write, IO_at_top.read, Caching_overhead.write, Caching_overhead.read, GC_overhead.write, GC_overhead.read, GC_overhead.erase, CMT_hit_cnt, CMT_miss_cnt, CMT_SIZE, gc_time, io_at_top_time, map_rw_time, total_time);
	printf("\nTotal eavaluation file writing complete.\n");
	//fclose(fp);
#endif
	fp = fopen(file_name, "r");
	if(fp == NULL){
		fp = fopen(file_name, "a+");
		if(FTL_FLAG == ULM)
			fprintf(fp, "FTL, BLOCK_per_UNIT, FLASH_MTB_PAGE, WRITE, READ, ERASE,IO_WRITE, IO_READ, LUN_MAP_LOAD, LUN_MAP_WRITE, UBN_MAP_LOAD, UBN_MAP_WRITE, intra.write, " 
			"intra.read, intra.erase, inter.write, inter.read, inter.erase, UBN_hit_cnt, UBN_miss_cnt, LUN_hit_cnt, LUN_miss_cnt, intra_GC_cnt, inter_GC_cnt, CMT_SIZE, tot_remain_space\n");
		else if(FTL_FLAG == DFTL)
			fprintf(fp, "FTL, FLASH_MTB_PAGE, WRITE, READ, ERASE, IO_WRITE, IO_READ, MAP_LOAD, MAP_WRITE, GC_overhead.write, GC_overhead.read, GC_overhead.erase, CMT_hit_cnt, CMT_miss_cnt, CMT_SIZE\n");
	}
	else{
		fclose(fp);
		fp=fopen(file_name, "a+");
	}
	if (FTL_FLAG == ULM){
		fprintf(fp, "ULM, ");
		fprintf(fp, "%d, ", BLOCK_per_UNIT);
		fprintf(fp, "%d, ", FLASH_MTB_PAGE);
		fprintf(fp, "%d, ", writecnt);
		fprintf(fp, "%d, ", readcnt);
		fprintf(fp, "%d, ", erasecnt);
		fprintf(fp, "%d, ", IO.write);
		fprintf(fp, "%d, ", IO.read);
		fprintf(fp, "%d, ", LUN_func.read);
		fprintf(fp, "%d, ", LUN_func.write);
		fprintf(fp, "%d, ", UBN_func.read);
		fprintf(fp, "%d, ", UBN_func.write);
		fprintf(fp, "%d, ", intra.write);
		fprintf(fp, "%d, ", intra.read);
		fprintf(fp, "%d, ", intra.erase);
		fprintf(fp, "%d, ", inter.write);
		fprintf(fp, "%d, ", inter.read);
		fprintf(fp, "%d, ", inter.erase);
		fprintf(fp, "%d, ", UBN_hit_cnt);
		fprintf(fp, "%d, ", UBN_miss_cnt);
		fprintf(fp, "%d, ", LUN_hit_cnt);
		fprintf(fp, "%d, ", LUN_miss_cnt);
		fprintf(fp, "%d, ", intra_GC_cnt);
		fprintf(fp, "%d, ", inter_GC_cnt);
		fprintf(fp, "%d, ", CMT_SIZE);
		fprintf(fp, "%llu\n", tot_remain_space);

	}
	else if (FTL_FLAG == DFTL){
		//fprintf(fp, "FTL, FLASH_MTB_PAGE, WRITE, READ, ERASE, IO_WRITE, IO_READ, MAP_LOAD, MAP_WRITE, GC_overhead.write, GC_overhead.read, GC_overhead.erase, CMT_hit_cnt, CMT_miss_cnt, CMT_SIZE\n");
		fprintf(fp, "DFTL, ");
		fprintf(fp, "%d, ", FLASH_MTB_PAGE);
		fprintf(fp, "%d, ", writecnt);
		fprintf(fp, "%d, ", readcnt);
		fprintf(fp, "%d, ", erasecnt);
		fprintf(fp, "%d, ", IO.write);
		fprintf(fp, "%d, ", IO.read);
		fprintf(fp, "%d, ", Caching_overhead.read);
		fprintf(fp, "%d, ", Caching_overhead.write);
		fprintf(fp, "%d, ", GC_overhead.write);
		fprintf(fp, "%d, ", GC_overhead.read);
		fprintf(fp, "%d, ", GC_overhead.erase);
		fprintf(fp, "%d, ", CMT_hit_cnt);
		fprintf(fp, "%d, ", CMT_miss_cnt);
		fprintf(fp, "%d\n", CMT_SIZE);
	}
	printf("\nTotal eavaluation file writing complete.\n");

	if (FTL_FLAG == ULM){
		sprintf(file_name, "%s_cl_info_ulm.txt", trace_addr);

		cl_info_fpout = fopen(file_name, "w+");

		fprintf(cl_info_fpout, "CL_NO, FREE_PAGE, INTRA_GC_CNT, INTRA_READ, INTRA_WRITE, INTRA_ERASE, INTER_GC_CNT, INTER_READ, INTER_WRITE, INTER_ERASE,"
			" BLK_ALLOC\n");

		for (iter = 0; iter < UNIT_per_FLASH; iter++){
			if (cl_info[iter].ifMAP == 1)
				continue;
			fprintf(cl_info_fpout, "%d, ", iter);
			fprintf(cl_info_fpout, "%d, ", cl_info[iter].free_page);
			fprintf(cl_info_fpout, "%d, %d, %d, %d, ", cl_info[iter].intra_GC_cnt, cl_info[iter].intraGC.read, cl_info[iter].intraGC.write, cl_info[iter].intraGC.erase);
			fprintf(cl_info_fpout, "%d, %d, %d, %d, ", cl_info[iter].inter_GC_cnt, cl_info[iter].interGC.read, cl_info[iter].interGC.write, cl_info[iter].interGC.erase);
			fprintf(cl_info_fpout, "%d\n", cl_info[iter].blk_alloc_cnt); 
		}
		
	}
#endif

	printf("\nTotal evaluation file writing writing complete.\n");
}

void valid_watcher(char *addr){

	int i,j;
	FILE *fp;
	char file_name[100];
	int validcnt;
	int stack_no; 

	validcnt = 0;

	if(FTL_FLAG == ULM)
		sprintf(file_name, "%s ULM block valid watcher.txt", addr);
	else if(FTL_FLAG == DFTL)
		sprintf(file_name, "%s DFTL block valid_watcher.txt", addr);

	fp = fopen(file_name, "a+");

	while(1){
		FB_top++;
		stack_no = FB_top - FB_LIMIT;
		validcnt = 0;
		if(FB_top > BLOCK_per_FLASH - 1)
			break;
		for(j=0; j<PAGE_per_BLOCK; j++){
			if(PBN[FB_stack[FB_top]].valid[j] == 1)
				validcnt++;
		}
		fprintf(fp, "%d, %d\n", stack_no, validcnt);
	}

	fclose(fp);
}
