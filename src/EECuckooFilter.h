#ifndef DYNAMICCUCKOOFILTER_H_
#define DYNAMICCUCKOOFILTER_H_


#include"cuckoofilter.h"
#include"linklist.h"
#include<list>
#include<math.h>
#include<iostream>
#include <queue>


class EECuckooFilter{
private:

	int capacity;
	int single_capacity;

	int single_table_length;

	double false_positive;
	double single_false_positive;

	double fingerprint_size_double;
	int fingerprint_size;

	Victim victim;

	CuckooFilter* curCF;
	CuckooFilter* nextCF; 

public:

	int counter;

	LinkList* cf_list;

	//construction & distruction functions
	EECuckooFilter(const size_t capacity, const double false_positive, const size_t exp_block_num = 4);
	~EECuckooFilter();

	//insert & query & delete functions
	bool insertItem(const char* item);
	CuckooFilter* getNextCF(CuckooFilter* curCF);
	bool failureHandle(Victim &victim);
	bool queryItem(const char* item);
	bool deleteItem(const char* item);

	//compaction
	bool compact();
	void sort(CuckooFilter** cfq, int queue_length);
	bool remove(CuckooFilter* cf_remove);

	//generate 2 bucket addresses
	void generateIF(const char* item, size_t &index, uint32_t &fingerprint, int fingerprint_size, int single_table_length);
	void generateA(size_t index, uint32_t fingerprint, size_t &alt_index, int single_table_length);

	//get info of DCF
	int getFingerprintSize();
	float size_in_mb();

	//extra function to make sure the table length is the power of 2
	uint64_t upperpower2(uint64_t x);

	void info();

	CuckooFilter *PtoCF[128];
	bool is_full;
	bool is_empty;
	int LevelHigh,LevelLow;
	int insertItemF(size_t index, uint32_t fingerprint, Victim &victim, bool Level_up);
	CuckooFilter* splitCF(CuckooFilter* PtoCFsplit,CuckooFilter* PNewCF,size_t indexinCF, uint32_t fingerprint);
	int getcount();
	int counter_ISc;	
	void coutDCF();
	size_t countCF;//
	int used[128];
	int CF_Level[128],countinSp[128],level[128];
	bool split_YON[128];
	uint32_t countSpB[128];
	int counterInsF;
	int findI;
	int capacityCF[10];
	bool insertItemNR(const char* item);
	bool insertItem(size_t index, uint32_t fingerprint);
	bool queryItemI(size_t index, uint32_t fingerprint);
	queue<int> HevLoad_Que;	
	};







#endif //DYNAMICCUCKOOFILTER_H
