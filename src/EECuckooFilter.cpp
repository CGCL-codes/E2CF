/*
 * 
 *
 *  Created on:
 *      Author: 
 */

#include "EECuckooFilter.h"
#include <chrono>
#include <queue>

using namespace std;
using namespace std::chrono;

EECuckooFilter::EECuckooFilter(const size_t item_num, const double fp, const size_t exp_block_num){

	capacity = item_num;
	LevelHigh = LevelLow = 0;
	
	single_table_length = upperpower2(capacity/4.0/exp_block_num);//2048 1024 512 256 128 ---!!!---must be the power of 2---!!!---  
	// single_table_length = 8;
	// single_table_length = 4194304;////in order to
	single_capacity = single_table_length*0.8*4;//s=6 1920 s=12 960 s=24 480 s=48 240 s=96 120

	false_positive = fp;
	single_false_positive = 1-pow(1.0-false_positive, ((double)single_capacity/capacity));

	fingerprint_size_double = ceil(log(8.0/single_false_positive)/log(2));
	if(fingerprint_size_double>0 && fingerprint_size_double<=4){
		fingerprint_size = 12;
	}else if(fingerprint_size_double>4 && fingerprint_size_double<=8){
		fingerprint_size = 12;
	}else if(fingerprint_size_double>8 && fingerprint_size_double<=12){
		fingerprint_size = 12;
	}else if(fingerprint_size_double>12 && fingerprint_size_double<=16){
		fingerprint_size = 12;
	}else if(fingerprint_size_double>16 && fingerprint_size_double<=24){
		fingerprint_size = 12;
	}else if(fingerprint_size_double>24 && fingerprint_size_double<=32){
		fingerprint_size = 12;
	}else{
		cout<<"fingerprint out of range!!!"<<endl;
		fingerprint_size = 16;
	}


	counter = 0;
	counter_ISc = 0;
	counterInsF = 0;

	curCF = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);
	nextCF = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);

	PtoCF[0] = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);
	// PtoCF[0]->is_full = false;
	countCF = 1;
	for(int i=0; i<128; i++){
		used[i] = 0;
		split_YON[i] = false;
	}
	used[0] = 1;
	for(int j = 0; j < 128; j++){		
		level[j] = -1;
	}
	level[0] = 0;
	for(int k = 0; k < 128;k++){
		countSpB[k] = 0;
	}
	for(int i=0; i<128; i++){
		countinSp[i] = 0;
	}
	findI = 0;
}

EECuckooFilter::~EECuckooFilter(){
	delete PtoCF[128];
}

bool EECuckooFilter::insertItemNR(const char* item){
	size_t index;
	uint32_t fingerprint;
	generateIF(item, index, fingerprint, fingerprint_size, single_table_length);
	if(queryItemI(index,fingerprint)){
		findI++;
		return true;
	}
	if(insertItem(index,fingerprint)){
		return true;
	}
	return false;
}


////
bool EECuckooFilter::insertItem(size_t index, uint32_t fingerprint){
	size_t  numCF, indexinCF,nPow_levelL,nPow_levelH;
	nPow_levelL = (size_t)pow(2,LevelLow);
	nPow_levelH = (size_t)pow(2,LevelHigh);
	numCF = (size_t)(index%nPow_levelL);
	
	if((PtoCF[numCF]->is_full == true) &&split_YON[numCF] == false && ((level[numCF] == LevelLow)||(countCF == nPow_levelH))&& numCF+nPow_levelL<=16 && countCF<16)
	{
		cout<<"here is spliting!*************************"<<endl;
		indexinCF = (size_t)(index/nPow_levelL);
		
		PtoCF[numCF+nPow_levelL] = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);

		PtoCF[numCF+nPow_levelL] = splitCF(PtoCF[numCF],PtoCF[numCF+nPow_levelL],indexinCF, fingerprint);
		counter_ISc++;
		split_YON[numCF] = true;
		split_YON[numCF+nPow_levelL] = true;
		used[numCF+nPow_levelL] = 1;
		

		countSpB[numCF]++;
		PtoCF[numCF]->is_full = false;
		PtoCF[numCF+nPow_levelL]->is_full = false;
		if(indexinCF%2 == 0){PtoCF[numCF]->counter = (single_capacity/2) + 1; PtoCF[numCF+nPow_levelL]->counter = single_capacity - (single_capacity/2);}
		else
		{
			PtoCF[numCF]->counter = single_capacity/2; PtoCF[numCF+nPow_levelL]->counter = single_capacity - (single_capacity/2) + 1;
		}
	}
	else if(split_YON[numCF] == true){
		indexinCF = (size_t)(index/pow(2,LevelLow));
		char* p = PtoCF[numCF]->getBucket(indexinCF*pow(2,LevelLow));
		int verP = *((uint8_t*)p);
		verP &= 0xf;
		if(verP == LevelLow){
	
			PtoCF[numCF+nPow_levelL] = splitCF(PtoCF[numCF],PtoCF[numCF+nPow_levelL],indexinCF,fingerprint);
			if(indexinCF%2 == 0)PtoCF[numCF]->counter++;
			else
			{
				PtoCF[numCF+nPow_levelL]->counter++;
			}
			countSpB[numCF]++;
			counter_ISc++;

			countinSp[numCF]++;
			if(PtoCF[numCF]->counter == single_capacity){
				PtoCF[numCF]->is_full = true;
			}
		}else{
			if(insertItemF(index,fingerprint,victim,true)){
				counter_ISc++;
				counter++;
				if(PtoCF[numCF]->counter == single_capacity){
					this->is_full = true;
				}
			}
			else{
					counterInsF++;
				}
			}
		if(countSpB[numCF] >= single_table_length/pow(2,LevelLow+1) || (numCF>nPow_levelL && countSpB[numCF-nPow_levelL] >= single_table_length/pow(2,LevelLow+1))){

			if(numCF < nPow_levelL){
				split_YON[numCF] = false;
				split_YON[numCF+nPow_levelL] = false;
				countSpB[numCF] = 0;
				countSpB[numCF+nPow_levelL] = 0;
				level[numCF+nPow_levelL] = LevelLow+1;
				level[numCF] = LevelLow+1;
			}
			else{
				split_YON[numCF] = false;
				split_YON[numCF-nPow_levelL] = false;
				countSpB[numCF] = 0;
				countSpB[numCF-nPow_levelL] = 0;
				level[numCF-nPow_levelL] = LevelLow+1;
				level[numCF] = LevelLow+1;
			}
			countCF++;
			if(countCF-1 == (size_t)pow(2,LevelLow)){
				LevelHigh++;
			}
			if(countCF==(size_t)pow(2,LevelLow+1)){
				LevelLow = LevelHigh;
			}

			cout<<"*******************>>>>>>>>>>"<<endl;
		}
	
	}
	else if(insertItemF(index,fingerprint,victim,false)){
	counter_ISc++;


		if(PtoCF[numCF]->counter > 0){
				PtoCF[numCF]->is_empty = false;
		}
		
	}else{
		if(failureHandle(victim) == false)counterInsF++;
		if(PtoCF[numCF]->counter == single_capacity){
			PtoCF[numCF]->is_full = true;
		}

		if(PtoCF[numCF]->counter > 0){
			PtoCF[numCF]->is_empty = false;
		}
	}
	return true;
}

//
CuckooFilter* EECuckooFilter::splitCF(CuckooFilter* PtoCFsplit,CuckooFilter* PNewCF,size_t indexinCF, uint32_t fingerprint){
	uint64_t aBucket,PowLevel,PowLevel_last;

	PowLevel = (size_t)pow(2,LevelHigh);///
	PowLevel_last = (size_t)pow(2,LevelLow);
	
	PtoCFsplit->is_full = false;
	if(indexinCF%2 == 0){
		for(size_t mov_bucket = 0; mov_bucket < PowLevel_last; mov_bucket++){
			if(mov_bucket == 0){
				PtoCFsplit->moveodd_addItem(LevelLow+1, indexinCF*PowLevel_last+mov_bucket, &aBucket,fingerprint);
				PNewCF->addBNewCF(LevelLow+1,indexinCF*PowLevel_last+mov_bucket, aBucket);
			}
			else{
				PtoCFsplit->moveodd_self_naive(LevelLow+1, (indexinCF+1)*PowLevel_last+mov_bucket, &aBucket);
				PNewCF->addBNewCF(LevelLow+1,indexinCF*PowLevel_last+mov_bucket, aBucket);
			}
		}
	}
	else{
		size_t mov_bucket = 0;
		for(; mov_bucket < PowLevel_last; mov_bucket++){
			PtoCFsplit->moveodd_self_naive(LevelLow+1, indexinCF*PowLevel_last+mov_bucket, &aBucket);
			PNewCF->addBNewCF(LevelLow+1,(indexinCF-1)*PowLevel_last+mov_bucket, aBucket);			
		}
		if(mov_bucket == PowLevel_last){
			PNewCF->addBNewCF_addItem(LevelLow+1,(indexinCF-1)*PowLevel_last+mov_bucket, fingerprint);
		}			
	}
	return PNewCF;
}

int EECuckooFilter::insertItemF(size_t index, uint32_t fingerprint, Victim &victim, bool level_up){
	size_t alt_index,numCF, indexinCF, nPow_level,nP_level_high;
	bool inser_SOF;	
	size_t count = 0;
	nPow_level = pow(2, LevelLow);
	numCF = index % nPow_level;
	nP_level_high = (size_t)pow(2,LevelLow+1);
	for(; count < MaxNumKicks; count++){
		bool kickout = (count != 0);
		if(split_YON[numCF] == false){
			if(level[numCF] == (LevelLow+1))
			{
				numCF = index%nP_level_high;
				indexinCF = index/nP_level_high;
				if(PtoCF[numCF]->insertImpl(indexinCF, fingerprint, nP_level_high, kickout, victim)){
					PtoCF[numCF]->counter++;
					if(PtoCF[numCF]->counter >= single_capacity-single_table_length*4*0.1){
						HevLoad_Que.push(numCF);
						if(HevLoad_Que.size() == nPow_level/4){
							PtoCF[HevLoad_Que.front()]->is_full = true;
							HevLoad_Que.pop();
						}
					}
					if(PtoCF[numCF]->counter >= single_capacity){
						PtoCF[numCF]->is_full = true;
					}
					return true;
				}
				if(kickout){
					fingerprint = victim.fingerprint;
					generateA(index, fingerprint, alt_index, single_table_length);
					numCF = alt_index%nPow_level;
					index = alt_index;
				}else{
					generateA(index, fingerprint, alt_index, single_table_length);
					numCF = alt_index%nPow_level;
					index = alt_index;
				}
			}
			else if(level[numCF] == LevelLow){
				numCF = index%nPow_level;
				indexinCF = index/nPow_level;
				if(PtoCF[numCF]->insertImpl(indexinCF, fingerprint, nPow_level, kickout, victim)){
					PtoCF[numCF]->counter++;
					if(PtoCF[numCF]->counter >= single_capacity-single_table_length*4*0.1){
						HevLoad_Que.push(numCF);
						if(HevLoad_Que.size() == nPow_level/4){
							PtoCF[HevLoad_Que.front()]->is_full = true;
							HevLoad_Que.pop();
						}
					}
					if(PtoCF[numCF]->counter >= single_capacity){
						PtoCF[numCF]->is_full = true;
					}
					return true;
				}
				if(kickout){
					fingerprint = victim.fingerprint;
					generateA(index, fingerprint, alt_index, single_table_length);
					numCF = alt_index%nPow_level;
					index = alt_index;
				}else{
					generateA(index, fingerprint, alt_index, single_table_length);
					numCF = alt_index%nPow_level;
					index = alt_index;
				}
			}
		}
		/////////////////////////////////
		else {////		
			indexinCF = index / nPow_level;
			char* p = PtoCF[numCF]->getBucket(indexinCF*nPow_level);
			int verP = *((uint8_t*)p);
			verP &= 0xf;
			if(verP == LevelLow){
				PtoCF[numCF+nPow_level] = splitCF(PtoCF[numCF],PtoCF[numCF+nPow_level],indexinCF,fingerprint);
				if(indexinCF%2 == 0)
				PtoCF[numCF]->counter++;
				else
				{	PtoCF[numCF+nPow_level]->counter++;	}		
				countSpB[numCF]++;
				return true;
			}
			numCF = index % nP_level_high;
			indexinCF = index/nP_level_high;
			inser_SOF = PtoCF[numCF]->insertImpl(indexinCF, fingerprint, nP_level_high, kickout, victim);
			// }			
			if(inser_SOF){
				PtoCF[numCF]->counter++;
				// cout<<"PtoCF[numCF]->counter="<<PtoCF[numCF]->counter<<endl;
				return true;
			} 
			if(kickout){
					fingerprint = victim.fingerprint;
					generateA(index, fingerprint, alt_index, single_table_length);
					numCF = alt_index%nPow_level;
					index = alt_index;
			}else{
				generateA(index, fingerprint, alt_index, single_table_length);
				numCF = alt_index%nPow_level;
				index = alt_index;
			}			
		}
	}
	return false;
}

bool EECuckooFilter::failureHandle(Victim &victim){
	size_t  numCF,  indexinCF, nPow_levelL;

	nPow_levelL = (size_t)pow(2,LevelLow);
	numCF = (size_t)(victim.index%nPow_levelL);
	indexinCF = (size_t)(victim.index/nPow_levelL);
	if(used[numCF+nPow_levelL] == 1)return false;
	if(split_YON[numCF] == true)return  false;	
	if(numCF+nPow_levelL>37)return true;
	cout<<"here is spliting! because failurehandle************"<<endl;
	PtoCF[numCF+nPow_levelL] = new CuckooFilter(single_table_length, fingerprint_size, single_capacity);

	PtoCF[numCF+nPow_levelL] = splitCF(PtoCF[numCF],PtoCF[numCF+nPow_levelL],indexinCF, victim.fingerprint);

	split_YON[numCF] = true;
	split_YON[numCF+nPow_levelL] = true;
	used[numCF+nPow_levelL] = 1;
	cout<<"countCF="<<countCF<<",new CF NO.="<<numCF+nPow_levelL<<endl;
	
	countSpB[numCF]++;
	PtoCF[numCF]->is_full = false;
	PtoCF[numCF+nPow_levelL]->is_full = false;
	if(indexinCF%2 == 0){PtoCF[numCF]->counter = (single_capacity/2) + 1; PtoCF[numCF+nPow_levelL]->counter = single_capacity - (single_capacity/2);}
	else
	{
		PtoCF[numCF]->counter = single_capacity/2; PtoCF[numCF+nPow_levelL]->counter = single_capacity - (single_capacity/2) + 1;
	}
	return true;
}

bool EECuckooFilter::queryItem(const char* item){
	size_t index, alt_index,nPow_level,numCF,indexinCF,nPow_lev_Last;
	uint32_t fingerprint;

	generateIF(item, index, fingerprint, fingerprint_size, single_table_length);
	nPow_level = 1<<LevelHigh;
	nPow_lev_Last = 1<<LevelLow;
	numCF = index&(nPow_level-1);
	for(int numQuery = 0; numQuery < 2; numQuery++){
		if((used[numCF]==1)&&(level[numCF] == LevelHigh)&&(split_YON[numCF] == false)){
			indexinCF = index/nPow_level; 
			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_level)){
				return true;
			}else if(numQuery == 0){                                              
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}
		else if((used[numCF] == 1) && (split_YON[numCF] == false)){		
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow;

			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
				return true;
			}else if(numQuery == 0){
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}else{
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow; 
			char* p = PtoCF[numCF]->getBucket(indexinCF*nPow_lev_Last);
			
			int verP = *((uint8_t*)p);
			verP &= 0xf;
			if(verP == LevelLow){
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
					return true;
				}else if(numQuery == 0){                                             
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index%nPow_level;
					// indexinCF = alt_index/nPow_lev_Last;
				}
			}else{
				size_t nPow_levelH = 1<<(LevelLow+1);
				numCF = index%nPow_levelH;
				indexinCF = index/nPow_levelH;
				
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_levelH)){
					return true;
				}else if(numQuery == 0){
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index&(nPow_level-1);
					// indexinCF = alt_index/nPow_lev_Last;
				}
			}
		}
	}
	return false;
}

bool EECuckooFilter::queryItemI(size_t index, uint32_t fingerprint){
	size_t alt_index,nPow_level,numCF,indexinCF,nPow_lev_Last;
	nPow_level = 1<<LevelHigh;
	nPow_lev_Last = 1<<LevelLow;
	numCF = index&(nPow_level-1);
	for(int numQuery = 0; numQuery < 2; numQuery++){
		if((used[numCF]==1)&&(level[numCF] == LevelHigh)&&(split_YON[numCF] == false)){
			indexinCF = index/nPow_level; 
			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_level)){
				return true;
			}else if(numQuery == 0){
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}
		else if((used[numCF] == 1) && (split_YON[numCF] == false)){
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow;

			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
				return true;
			}else if(numQuery == 0){
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}else{
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow; 
			char* p = PtoCF[numCF]->getBucket(indexinCF*nPow_lev_Last);
			
			int verP = *((uint8_t*)p);
			verP &= 0xf;
			if(verP == LevelLow){
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
					return true;
				}else if(numQuery == 0){
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index%nPow_level;
				}
			}else{
				size_t nPow_levelH = 1<<(LevelLow+1);
				numCF = index%nPow_levelH;
				indexinCF = index/nPow_levelH;
				
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_levelH)){
					return true;
				}else if(numQuery == 0){
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index&(nPow_level-1);
				}
			}
		}
	}
	return false;
}

bool EECuckooFilter::deleteItem(const char* item){

	size_t index, alt_index,nPow_level,numCF,indexinCF,nPow_lev_Last;
	uint32_t fingerprint;

	generateIF(item, index, fingerprint, fingerprint_size, single_table_length);
	nPow_level = 1<<LevelHigh;
	nPow_lev_Last = 1<<LevelLow;
	numCF = index&(nPow_level-1);
	for(int numQuery = 0; numQuery < 2; numQuery++){
		if((used[numCF]==1)&&(level[numCF] == LevelHigh)&&(split_YON[numCF] == false)){
			indexinCF = index/nPow_level; 
			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_level)){
				if(PtoCF[numCF]->deleteImpl(indexinCF, fingerprint,nPow_level)){
					PtoCF[numCF]->counter--;
					return true;
				}
			}else if(numQuery == 0){ 
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}
		else if((used[numCF] == 1) && (split_YON[numCF] == false)){			
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow;

			if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
				if(PtoCF[numCF]->deleteImpl(indexinCF, fingerprint,nPow_lev_Last)){
					PtoCF[numCF]->counter--;
					return true;
				}
			}else if(numQuery == 0){
				generateA(index, fingerprint, alt_index, single_table_length);
				index = alt_index;
				numCF = alt_index&(nPow_level-1);
			}
		}else{
			numCF = index&(nPow_lev_Last-1);
			indexinCF = index>>LevelLow; 
			char* p = PtoCF[numCF]->getBucket(indexinCF*nPow_lev_Last);
			
			int verP = *((uint8_t*)p);
			verP &= 0xf;
			if(verP == LevelLow){
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_lev_Last)){
					if(PtoCF[numCF]->deleteImpl(indexinCF, fingerprint,nPow_lev_Last)){
					PtoCF[numCF]->counter--;
					return true;
				}
				}else if(numQuery == 0){
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index%nPow_level;
				
				}
			}else{
				size_t nPow_levelH = 1<<(LevelLow+1);
				numCF = index%nPow_levelH;
				indexinCF = index/nPow_levelH;
				
				if(PtoCF[numCF]->queryImpl(indexinCF, fingerprint,nPow_levelH)){
					if(PtoCF[numCF]->deleteImpl(indexinCF, fingerprint,nPow_levelH)){
					PtoCF[numCF]->counter--;
					return true;
				}
				}else if(numQuery == 0){
					generateA(index, fingerprint, alt_index, single_table_length);
					index = alt_index;
					numCF = alt_index&(nPow_level-1);
					
				}
			}
		}
	}
	return false;

}



void EECuckooFilter::sort(CuckooFilter** cfq, int queue_length){
	CuckooFilter* temp;
	for(int i = 0; i<queue_length-1; i++){
		for(int j = 0; j<queue_length-1-i; j++){
			if(cfq[j]->counter > cfq[j+1]->counter){
				temp = cfq[j];
				cfq[j] = cfq[j+1];
				cfq[j+1] = temp;
			}
		}
	}
}



void EECuckooFilter::generateIF(const char* item, size_t &index, uint32_t &fingerprint, int fingerprint_size, int single_table_length){

	std::string  value = HashFunc::sha1(item);
	// cout<<"item:"<<item<<endl;
	// cout<<"value:"<<value<<endl;
	uint64_t hv = *((uint64_t*) value.c_str());
	// cout<<"hv:"<<hex<<hv<<endl;
	index = ((uint32_t) (hv >> 32)) % single_table_length;
	// cout<<"hv:"<<hex<<hv<<endl;
	fingerprint = (uint32_t) (hv & 0xFFFFFFFF);
	// cout<<"hv:"<<hex<<fingerprint<<endl;
	fingerprint &= ((0x1ULL<<fingerprint_size)-1);
	// cout<<"hv:"<<hex<<fingerprint<<endl;
	fingerprint += (fingerprint == 0);
	// cout<<"fp:"<<hex<<fingerprint<<endl;
}

void EECuckooFilter::generateA(size_t index, uint32_t fingerprint, size_t &alt_index, int single_table_length){
	alt_index = (index ^ (fingerprint * 0x5bd1e995)) % single_table_length;
}////////



int EECuckooFilter::getFingerprintSize(){
	return fingerprint_size;
}
//
float EECuckooFilter::size_in_mb(){
	return fingerprint_size * 4.0 * single_table_length * 1 / 8 / 1024 / 1024;
}

uint64_t EECuckooFilter::upperpower2(uint64_t x) {
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  x++;
  return x;
}
