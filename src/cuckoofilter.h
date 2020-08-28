#ifndef CUCKOOFILTER_H_
#define CUCKOOFILTER_H_

#include<string.h>
#include<stdlib.h>
#include"hashfunction.h"
#include"bithack.h"
#include"stdint.h"

#define MaxNumKicks 40

using namespace std;

typedef struct {
	size_t index;
	uint32_t fingerprint;
} Victim;

typedef struct{
	char* bit_array;
} Bucket;



class CuckooFilter{
private:

	int capacity;
	size_t single_table_length;
	size_t fingerprint_size;
	size_t bits_per_bucket;
	size_t bytes_per_bucket;

	Bucket* bucket;

	uint32_t mask;
//myself
	size_t tags_per_bucket;
public:

	bool is_full;
	bool is_empty;
	int counter;
	CuckooFilter* next;
	CuckooFilter* front;
	
	//construction and distruction function
	CuckooFilter(const size_t single_table_length, const size_t fingerprint_size, const int capacity);
	~CuckooFilter();

	//insert & query & delete function
	int  insertItem(const char* item, Victim &victim);
	bool insertItem(const size_t index, const uint32_t fingerprint, bool kickout, Victim &victim);
	bool queryItem(const char* item);
	// bool deleteItem(const char* item);

	bool insertImpl(const size_t index, const uint32_t fingerprint, const bool kickout, Victim &victim);
	bool queryImpl(const size_t index, const uint32_t fingerprint,size_t PowLevel);
	bool deleteImpl(const size_t index, const uint32_t fingerprint,size_t nPowlevel);

	//generate two candidate bucket addresses
	// void generateIF(const char* item, size_t &index, uint32_t &fingerprint, int fingerprint_size, int single_table_length);
	void generateA(size_t index, uint32_t fingerprint, size_t &alt_index, int single_table_length);

	//read from bucket & write into bucket
	uint32_t read(const size_t index, const size_t pos, size_t Pow_level);
	void write(const size_t index, const size_t pos, const uint32_t fingerprint, size_t Pow_level);

	//move corresponding fingerprints to sparser CF
	bool transfer(CuckooFilter* tarCF);
	bool insertImpl(const size_t index, const uint32_t fingerprint, size_t Pow_level, const bool kickout, Victim &victim);

	//

	uint64_t* moveodd_self_naive(int versionID,int iBucket,uint64_t* aBucket);
	uint64_t* moveodd_addItem(int versionID,int iBucket, uint64_t* aBucket,uint32_t fingerprint);
	void addBNewCF(size_t verID,size_t ibucket,uint64_t aBucket);
	void addBNewCF_naive(int iBucket,uint64_t aBucket);
	void addBNewCF_addItem(int versionID,size_t iBucket,uint32_t fingerprint);
	char* getBucket(uint32_t index);
	int Level;
};



#endif //CUCKOOFILTER_H_
