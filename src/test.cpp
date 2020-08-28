#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <vector>
#include "EECuckooFilter.h"
#include "cuckoofilter.h"
#include <chrono>

using namespace std;
using namespace std::chrono;

typedef struct{
	size_t item_num;
	double exp_FPR;
	string dataset_path;
}Config;


typedef struct{
	int exp_BBN;
	double actual_FPR;
	int actual_BBN;
	int F_size;
	double space_cost;
	double I_time0, I_time[30];
	double Q_time;
	double D_time;
	double C_rate;
}Metric;


Metric test(EECuckooFilter *dcf, const Config config, string *data, string *data_Q){

	Metric metric;
	cout<<" config.item_num="<<config.item_num<<endl;
	//**********insert**********
	int countT=0, itime=0;
	
	for(size_t i = 0; i < config.item_num; i++){
		dcf->insertItemNR(data[i].c_str());
		if(countT == 0)metric.I_time0 = clock();
		countT++;
		if(countT == 20000000){
			metric.I_time0 = clock() - metric.I_time0;
			metric.I_time[itime++] = metric.I_time0/CLOCKS_PER_SEC;
			countT = 0;
		}
	}
	

	//**********query**********
	int false_positive_count = 0;
	int querysuc=0;
	//cout<<"????----?"<<endl;

    metric.Q_time = clock();
	for(int i = 0;i < 100; i++){
		if(dcf->queryItem(data_Q[i].c_str()) == true){
			querysuc++;
		}
	}
	metric.Q_time = clock() - metric.Q_time;
	metric.Q_time = metric.Q_time/CLOCKS_PER_SEC;
 
	cout<<"query success number is:"<<dec<<querysuc<<endl;


	metric.actual_FPR = (double)false_positive_count/config.item_num;


	//**********delete**********


	size_t count = 0;
	metric.D_time = clock();
	while(count < config.item_num){
		dcf->deleteItem(data[count].c_str());
		count += 1; //delete all the items
	}
	metric.D_time = clock() - metric.D_time;
	metric.D_time = metric.D_time/CLOCKS_PER_SEC;


	return metric;

}



string Get_Value(string config_buff){
	string value;
	int pos = config_buff.find("=", 0);
	if (pos != -1)
	{
		pos++;
		value = config_buff.substr(pos, config_buff.length());
	} else {
		exit(1);
	}

	while(1){
		pos = value.find(" ", 0);
		if(pos >= 0){
			value = value.substr(pos+1, config_buff.length());
		}
		else{
			break;
		}
	}

	return value;
}


Config Read_Config(const string path){
	ifstream in_config(path.c_str());
	string config_buff;
	Config configuration;
	getline(in_config, config_buff);
	configuration.exp_FPR = atof(Get_Value(config_buff).c_str());
	getline(in_config, config_buff);
	configuration.item_num = atof(Get_Value(config_buff).c_str());
	getline(in_config, config_buff);
	configuration.dataset_path = Get_Value(config_buff);

	return configuration;
}


string* Read_Dataset(const Config config, const string path){
	ifstream in_input(path.c_str());
	string input_buff;
	if(!getline(in_input, input_buff)){
		cout << "Read File Error!"<<config.dataset_path << endl;
	}
	string buff;
	stringstream ss(input_buff);

	vector<string> tokens;
	size_t item_count = 0;
	string *input_data = new string[config.item_num];
	while (item_count < config.item_num){
	//	getline(in_input, buff);
	        ss >> buff;
		tokens.push_back(buff);
		input_data[item_count] = buff;
		item_count ++;
	}
	return input_data;
}

void Print_Info(Config config, Metric metric){

	ofstream out("./result/result.txt",iostream::app);
	time_t timep;
	
	struct tm *lt;
	time (&timep);
	lt = localtime(&timep);
	out<<"mon:"<<lt->tm_mon<<",day:"<<lt->tm_mday<<",time:"<<lt->tm_hour<<":"<<lt->tm_min<<":"<<lt->tm_sec<<endl;

        out << setw(15) << "item_num" << setw(15) << "exp_FPR"
		<< setw(15) << "actual_FPR" << setw(15) << "actual_BBN" << setw(15) << "F_size(bits)"
		<< setw(15) << "space_cost(MB)"
		<< setw(15) << "Q_time(s)" << setw(15) << "D_time(s)" << setw(10) << "C_rate"
		<< endl;
	out << setw(15) << "I_time0(s)"<<setw(15) << "I_time1(s)" <<setw(15) << "I_time2(s)"<<setw(15) << "I_time3(s)"
	    << setw(15) << "I_time4(s)"<<setw(15) << "I_time5(s)" <<setw(15) << "I_time6(s)"<<setw(15) << "I_time7(s)"
		<< setw(15) << "I_time8(s)"<< setw(15) << "I_time9(s)"<< setw(15)<< "I_time10(s)"<<setw(15) << "I_time11(s)"<<endl;

	out << setw(15) << config.item_num << setw(15) << config.exp_FPR
		<< setw(15) << metric.actual_FPR << setw(15) << metric.actual_BBN << setw(15) << metric.F_size
		<< setw(15) << metric.space_cost
		<< setw(15) << metric.Q_time << setw(15) << metric.D_time << setw(10) << metric.C_rate
		<< endl;
	out << setw(15) << metric.I_time[0]<< setw(15) << metric.I_time[1] <<setw(15) << metric.I_time[2]<< setw(15) << metric.I_time[3]
	    << setw(15) << metric.I_time[4]<< setw(15) << metric.I_time[5] <<setw(15) << metric.I_time[6]<< setw(15) << metric.I_time[7]
		<< setw(15) << metric.I_time[8]<< setw(15) << metric.I_time[9] <<setw(15) << metric.I_time[10]<<setw(15) << metric.I_time[11] << endl;
}




int main(int argc, char* argv[]){

	Config config[10];
	string *data[10];

	string config_path = "./configuration/config.txt";
	config[0] = Read_Config(config_path);

	string dataset_path = config[0].dataset_path;
	data[0] = Read_Dataset(config[0], dataset_path);

	string config_path_Q = "./configuration/config_Q.txt";
	Config config_Q = Read_Config(config_path_Q);

	string dataset_path_Q = config_Q.dataset_path;
	string *data_Q = Read_Dataset(config_Q, dataset_path_Q);
	
	EECuckooFilter* dcf = new EECuckooFilter(config[0].item_num, config[0].exp_FPR);
	for(int i = 0; i < 1; i++){
		Metric metric = test(dcf,config[i],data[i],data_Q);
		Print_Info(config[i], metric);
	}
}
