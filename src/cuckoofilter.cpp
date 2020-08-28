/*
 *
 *      Author:
 */

#include "cuckoofilter.h"
#include<iostream>
#include<math.h>
using namespace std;

CuckooFilter::CuckooFilter(const size_t table_length, const size_t fingerprint_bits, const int single_capacity){
	fingerprint_size = fingerprint_bits;
	tags_per_bucket = 4;
	bits_per_bucket = fingerprint_size*tags_per_bucket+4;
	bytes_per_bucket = (fingerprint_size*tags_per_bucket+4+7)>>3;
	single_table_length = table_length;
	counter = 0;
	capacity = single_capacity;
	is_full = false;
	is_empty = true;
	next = NULL;
	front = NULL;
	mask = (1ULL << fingerprint_size) - 1;

	bucket = new Bucket[single_table_length];
	char* totalB = new char[bytes_per_bucket*single_table_length];

	for(size_t i = 0; i<single_table_length; i++){
		bucket[i].bit_array = totalB+(bytes_per_bucket*i);
		memset(bucket[i].bit_array, 0, bytes_per_bucket);
	}
}

CuckooFilter::~CuckooFilter(){
	delete[] bucket;
	delete next;
	delete front;
}



bool CuckooFilter::insertImpl(const size_t index, const uint32_t fingerprint,size_t Pow_level,  const bool kickout, Victim &victim){
	size_t T_PBucket_PeDCF;
	T_PBucket_PeDCF = tags_per_bucket * Pow_level;
	
	for(size_t pos = 0; pos<T_PBucket_PeDCF; pos++){
		if(read(index,pos,Pow_level) == 0){
			write(index,pos,fingerprint,Pow_level);
			
			if(this->counter > 0 ){
				this->is_empty = false;
			}
			return true;
		}
	}
	if(kickout){
		int j = rand()%T_PBucket_PeDCF;
		victim.index = index;
		victim.fingerprint = read(index,j,Pow_level);
		write(index,j,fingerprint,Pow_level);
	}
	return false;
}

bool CuckooFilter::queryImpl(const size_t index, const uint32_t fingerprint,size_t PowLevel){
	if(fingerprint_size == 4){
		const char* p = bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;

		return hasvalue4(bits, fingerprint);
	}else if(fingerprint_size == 8){
		const char* p = bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;

		return hasvalue8(bits, fingerprint);
	}else if(fingerprint_size == 12){
		for(uint32_t nBucket =0; nBucket < PowLevel; nBucket++){
			char* p = bucket[index*PowLevel+nBucket].bit_array;
			uint64_t bits = *(uint64_t*)p;
			if(hasvalue12(bits, fingerprint)){
				return true;
			}
		}
	}else if(fingerprint_size == 16){
		const char* p = bucket[index].bit_array;
		uint64_t bits = *(uint64_t*)p;

		return hasvalue16(bits, fingerprint);
	}else{
		return false;
	}
}

bool CuckooFilter::deleteImpl(const size_t index, const uint32_t fingerprint, size_t nPowlevel){
	size_t T_PBucket_PeDCF;
	T_PBucket_PeDCF = tags_per_bucket * nPowlevel;
	for(size_t pos = 0; pos<T_PBucket_PeDCF; pos++){
		if(read(index, pos, nPowlevel) == fingerprint){
			// cout<<"------"<<endl;
			write(index, pos, 0,nPowlevel);
			counter--;
			if(counter < this->capacity){
				this->is_full = false;
			}
			if(counter == 0){
				this->is_empty = true;
			}
			return true;
		}
	}
	return false;
}


void CuckooFilter::generateA(size_t index, uint32_t fingerprint, size_t &alt_index, int single_table_length){
	alt_index = (index ^ (fingerprint * 0x5bd1e995)) % single_table_length;
}



uint32_t CuckooFilter::read(size_t index, size_t pos,size_t Pow_level){
	const char* p = bucket[index*Pow_level].bit_array;
	uint32_t fingerprint;
	p += ((size_t)(pos/tags_per_bucket))*bytes_per_bucket;
	pos = (size_t)(pos%tags_per_bucket);
	if(fingerprint_size == 4){
		p += (pos >> 1);
		uint8_t bits_8 = *(uint8_t*)p;
		if((pos & 1) == 0){
			fingerprint = (bits_8>>4) & 0xf;
		}else{
			fingerprint = bits_8 & 0xf;
		}
	}else if(fingerprint_size == 8){
		p += pos;
		uint8_t bits_8 = *(uint8_t*)p;
		fingerprint = bits_8 & 0xff;
	}else if(fingerprint_size == 12){
		p += pos+(pos>>1); 
		uint32_t bits_32 = *(uint32_t*)p;
		if(pos== 0){
			fingerprint = (bits_32 >> 4) & 0xfff;
		}else if(pos == 1) {
			p += 1;
			bits_32 = *(uint32_t*)p;
			fingerprint = bits_32 & 0xfff;
		}else if(pos == 2){
			fingerprint = (bits_32>>4) & 0xfff;
		}else
		{
			p += 1;
			bits_32 = *(uint32_t*)p;
			fingerprint = bits_32 & 0xfff;
		}
		
	}else if(fingerprint_size == 16){
		p += (pos<<1);
		uint16_t bits_16 = *(uint16_t*)p;
		fingerprint = bits_16 & 0xffff;
	}else if(fingerprint_size == 24){
		p += pos+(pos<<1);
		uint32_t bits_32 = *(uint32_t*)p;
		fingerprint = (bits_32 >> 4);
	}else if(fingerprint_size == 32){
		p += (pos<<2);
		uint32_t bits_32 = *(uint32_t*)p;
		fingerprint = bits_32 & 0xffffffff;
	}else{
		fingerprint =0;
	}
	return fingerprint & mask;
}

void CuckooFilter::write(size_t index, size_t pos, uint32_t fingerprint,size_t PowLevel){
	char* p = bucket[index*PowLevel].bit_array;
	p += ((size_t)(pos/tags_per_bucket))*bytes_per_bucket;
	pos = (size_t)(pos%tags_per_bucket);
	if(fingerprint_size == 4){
		p += (pos>>1);
		if((pos & 1) == 0){
			*((uint8_t*)p) &= 0x0f;
			*((uint8_t*)p) |= (fingerprint<<4);
		}else{
			*((uint8_t*)p) &= 0xf0;
			*((uint8_t*)p) |= fingerprint;
		}
	}else if(fingerprint_size == 8){
		((uint8_t*)p)[pos] = fingerprint;
	}else if(fingerprint_size == 12){
		p += (pos + (pos>>1));
		if(pos == 0){
			*((uint16_t*)p) &= 0x000f; //Little-Endian
			*((uint16_t*)p) |= fingerprint<<4;
		}else if (pos == 1)
		{
			p += 1;
			*((uint16_t*)p) &= 0xf000;
			*((uint16_t*)p) |= fingerprint;
		}else if (pos == 2)
		{
			*((uint32_t*)p) &= 0xffff000f;
			*((uint32_t*)p) |= (fingerprint<<4);
		}else{
			p += 1;
			*((uint16_t*)p) &= 0xf000;
			*((uint16_t*)p) |= fingerprint;
		}
	}else if(fingerprint_size == 16){
		((uint16_t*) p)[pos] = fingerprint;
	}else if(fingerprint_size == 24){
		p += (pos+ (pos<<1));
		*((uint32_t*)p) &= 0xff000000; //Little-Endian
		*((uint32_t*)p) |= fingerprint;
	}else if(fingerprint_size == 32){
		((uint32_t*) p)[pos] = fingerprint;
	}
}


uint64_t* CuckooFilter::moveodd_self_naive(int versionID,int iBucket, uint64_t* aBucket){
	int iBuck_last;
	iBuck_last = iBucket-(int)pow(2,versionID-1);
	char* p =  bucket[iBuck_last].bit_array;
	
	*((uint8_t*)p) &= 0xf0;
	*((uint8_t*)p) |= versionID%16;
	
	char* p1 = bucket[iBucket].bit_array;
	uint64_t bits64 = *(uint64_t*)p1;
	bits64 &= 0xffffffffffff0;
	bits64 |= versionID;
	*aBucket = bits64 & 0xfffffffffffff; //


	*((uint64_t*)p1) &= 0xffff000000000000;//
	*((uint8_t*)p1) |= versionID%16;
	return aBucket;
}
//
void CuckooFilter::addBNewCF_naive(int ibucket,uint64_t aBucket){
	char* p = bucket[ibucket].bit_array;
	*((uint64_t*)p) &= 0xf0000000000000;
	*((uint64_t*)p) |= aBucket;
}

uint64_t* CuckooFilter::moveodd_addItem(int versionID,int iBucket, uint64_t* aBucket,uint32_t fingerprint){
	char* p =  bucket[iBucket].bit_array;
	*((uint8_t*)p) &= 0xf0;
	*((uint8_t*)p) |= versionID%16;

	iBucket += pow(2,versionID-1);
	char* p1 = bucket[iBucket].bit_array;
	*((uint8_t*)p1) &= 0xf0;
	*((uint8_t*)p1) |= versionID%16;
	uint64_t bits64 = *(uint64_t*)p1;

	*aBucket = bits64 & 0xfffffffffffff; //


	*((uint64_t*)p1) &= 0xffff00000000000f;//
	*((uint32_t*)p1) |= fingerprint<<4;
	return aBucket;
}

void CuckooFilter::addBNewCF(size_t verID,size_t ibucket,uint64_t aBucket){
	//int powerL=pow(2,verID-1);
	char* p = bucket[ibucket].bit_array;
	*((uint64_t*)p) &= 0xf0000000000000;
	*((uint64_t*)p) |= aBucket;
}

void CuckooFilter::addBNewCF_addItem(int versionID,size_t iBucket,uint32_t fingerprint){
	char* p = bucket[iBucket].bit_array;
	*((uint64_t*)p) &= 0xfff0000000000000;
	
	*((uint64_t*)p) |= fingerprint<<4;//
	*(uint8_t*)p &= 0xf0;//
	*((uint8_t*)p) |= versionID%16;
}

char* CuckooFilter::getBucket(uint32_t index){
	return bucket[index].bit_array;
}
