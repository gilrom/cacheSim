/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include "cache_sim.h"

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

/*Implementation for classes*/

Cache::Cache(uint size, uint associativ, uint block_size) : size(size),
 num_of_calls(0), num_of_miss(0), assoc(associativ)
{
	
	num_of_set_bits = size - (block_size + assoc);
	num_of_tag_bits = 32 -2 -block_size -num_of_set_bits;

	uint num_of_set = 1 << num_of_set_bits;
	uint num_of_Ways = 1 << assoc;

	memory = new std::vector<Block*>[num_of_set];

	for(int set = 0 ; set < num_of_set ; set++)
	{
		memory[set] = std::vector<Block*>(num_of_Ways);
		for (int way = 0 ; way < num_of_Ways ; way++)
		{
			memory[set][way] = new Block;
			memory[set][way]->setLruKey(way);
			memory[set][way]->way = way;
		}
	}
}

Cache::~Cache()
{
	uint num_of_set = 1 << num_of_set_bits;
	uint num_of_Ways = 1 << assoc;
	for(int set = 0 ; set < num_of_set ; set++)
	{
		for (int way = 0 ; way < num_of_Ways ; way++)
		{
			delete memory[set][way];
		}
	}

	delete[] memory;
}

void Cache::parseSetAndTag(uint32_t addr, uint* tag, uint* set){

	uint64_t tag64, set64;
	uint64_t ThirtyTwoBitsMask = 0xFFFFFFFF;

	tag64 = addr >> (32 - num_of_tag_bits);
	*tag = tag64;

	set64 = addr << num_of_tag_bits;
	set64 = set64 & ThirtyTwoBitsMask;
	set64 = set64 >> (32 - num_of_set_bits);
	*set = set64;
}

void Cache::updateLru(uint way, uint set)
{
	int num_of_Ways = 1 << assoc;

	int x = memory[set][way]->getLruKey();
	memory[set][way]->setLruKey(num_of_Ways - 1);

	for (int cu_way = 0; cu_way < num_of_Ways; cu_way++)
	{
		if(cu_way != way && memory[set][cu_way]->getLruKey() > x)
		{
			int old_lru_key = memory[set][cu_way]->getLruKey();
			memory[set][cu_way]->setLruKey(old_lru_key - 1);
		}
	}
	/* lru testing:
	std::cout<<"printing the lru keys: "<<endl;
	std::cout<<"just changed: "<<way<<endl;
	for (int cu_way = 0; cu_way < num_of_Ways; cu_way++)
	{
		std::cout<<memory[set][cu_way]->getLruKey()<<endl;
	}
	*/

}

void Cache::invalidate (uint32_t addr)
{
	//std::cout<<"invalidate is ";
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	int num_of_Ways = 1 << assoc;

	for (int way = 0; way < num_of_Ways ; way++)
	{
		if(!(memory[set][way]->valid))
		{
			continue;
		}
		else if(memory[set][way]->tag == tag)
		{
			//std::cout<<"RIGHT!"<<endl;
			memory[set][way]->valid = false;	
			return;
		}
	}
}

bool Cache::snoop(uint32_t addr)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	int num_of_Ways = 1 << assoc;

	for (int way = 0; way < num_of_Ways ; way++)
	{
		if(!(memory[set][way]->valid))
		{
			continue;
		}
		else if(memory[set][way]->tag == tag)
		{
			memory[set][way]->valid = false;	
			return memory[set][way]->dirty;
		}
	}

	return false;
}

bool Cache::writeReq(uint32_t addr, bool realReq)
{
	if (realReq) num_of_calls++;

	uint tag, set;
	parseSetAndTag(addr, &tag, &set);
	
	int num_of_Ways = 1 << assoc;

	for (int way = 0; way < num_of_Ways ; way++)
	{
		if(!(memory[set][way]->valid))
		{
			continue;
		}
		else if(memory[set][way]->tag == tag)
		{
			memory[set][way]->dirty = true;
			updateLru(way,set);
			return true;
		}
	}
	if (realReq) num_of_miss++;
	return false;
}

bool Cache::readReq(uint32_t addr)
{
	num_of_calls++;
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);
	
	int num_of_Ways = 1 << assoc;

	for (int way = 0; way < num_of_Ways ; way++)
	{
		if(!(memory[set][way]->valid))
		{
			continue;
		}
		else if(memory[set][way]->tag == tag)
		{
			updateLru(way,set);
			return true;
		}
	}
	num_of_miss++;
	return false;
}

//on work
Block* Cache::selectVictim(uint32_t addr)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	int num_of_Ways = 1 << assoc;

	int min_lru_key = num_of_Ways;
	int index = 0;
	
	for (int way = 0; way < num_of_Ways ; way++)
	{
		if(memory[set][way]->valid)
		{
			if(min_lru_key > memory[set][way]->getLruKey())
			{
				min_lru_key = memory[set][way]->getLruKey();
				index = way;
			}
		}
		else
		{
			return memory[set][way];
		}
	}
	return memory[set][index];
}


void Cache::fillData(uint32_t addr, uint way)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);


	Block* to_insert = new Block;
	to_insert->addr = addr;
	int old_lru_key = memory[set][way]->getLruKey();
	to_insert->setLruKey(old_lru_key);
	to_insert->tag = tag;
	to_insert->valid = true;
	to_insert->way = way;

	//std::cout<<"where is it??"<<endl;

	if(memory[set][way]->valid)
	{
		std::cout<<"somthing is wrong"<<endl;
	}
	else
	{
		//std::cout<<"you are GOOD :)"<<endl;
	}
	*memory[set][way] = *to_insert;
	//std::cout<<"here"<<endl;
	updateLru(way, set);
	delete to_insert;
}

double Cache::getMissRate()
{
	return double(num_of_miss) / double(num_of_calls);
}

uint Cache::getNumOfAcc(){
	return num_of_calls;
}

void Cache::printCache()
{
	uint num_of_set = 1 << num_of_set_bits;
	uint num_of_Ways = 1 << assoc;

	for(int set = 0 ; set < num_of_set ; set++)
	{
		std::cout<<"set "<<set<<":";
		for (int way = 0 ; way < num_of_Ways ; way++)
		{
			if(memory[set][way]->valid)
			{
				std::cout<<"way "<<way<<": tag-"<<memory[set][way]->tag;
				if (memory[set][way]->dirty)
				{
					std::cout<<"dirty";
				}
			}
		}
		std::cout<<endl;
	}
}

CacheSim::CacheSim(uint MemCyc, uint BSize, uint L1Size, uint L2Size, uint L1Assoc,
			uint L2Assoc, uint L1Cyc, uint L2Cyc, uint WrAlloc) :

	alloc(WrAlloc), block_size(BSize), mem_cyc(MemCyc), num_of_mem_acc(0), 
	L1Cyc(L1Cyc), L2Cyc(L2Cyc),
	l1(L1Size, L1Assoc, BSize),	
	l2(L2Size, L2Assoc, BSize)
{}

double CacheSim::getL1MissRate()
{
	return l1.getMissRate();
}
double CacheSim::getL2MissRate()
{
	return l2.getMissRate();
}

double CacheSim::avgAccTime()
{
	double total_time = (l1.getNumOfAcc() * L1Cyc) + 
						(l2.getNumOfAcc() * L2Cyc) + 
			 			(num_of_mem_acc * mem_cyc);
	double total_calls = (l1.getNumOfAcc()+l2.getNumOfAcc()+num_of_mem_acc);
	return total_time / total_calls;
}

void CacheSim::read(uint32_t addr){
	std::cout<<"memory picture:"<<endl;
	std::cout<<"L1:"<<endl;
	l1.printCache();
	std::cout<<"L2:"<<endl;
	l2.printCache();
	if(!l1.readReq(addr)){
		if(!l2.readReq(addr)){
			num_of_mem_acc++;
			Block* b = l2.selectVictim(addr);
			if(!b->valid){ //cache is not full
				l2.fillData(addr, b->way);
			}
			else{
				bool dirty_in_l1 = l1.snoop(b->addr);
				if(dirty_in_l1){
					l2.writeReq(b->addr);
				}
				if(b->dirty){
					//write back to memory
				}
				l2.invalidate(b->addr);
				l2.fillData(addr, b->way);
			}
			b = l1.selectVictim(addr);
			if(!b->valid){
				l1.fillData(addr, b->way);
			}
			else{
				if(b->dirty){
					l2.writeReq(b->addr);
				}
				l1.invalidate(b->addr);
				l1.fillData(addr, b->way);
			}
		}
		else{
			Block* b = l1.selectVictim(addr);
			if(!b->valid){
				l1.fillData(addr, b->way);
			}
			else{
				if(b->dirty){
					l2.writeReq(b->addr);
				}
				l1.invalidate(b->addr);
				l1.fillData(addr, b->way);
			}
		}
	}
}

void CacheSim::write(uint32_t addr){
	if(!l1.readReq(addr)){
		if(!l2.readReq(addr)){
			num_of_mem_acc++;
			if(alloc)
			{
				Block* victim = l2.selectVictim(addr);
				if(!victim->valid){
					l2.fillData(addr, victim->way);
				}
				else{
					bool dirty_in_l1 = l1.snoop(victim->addr);
					if(dirty_in_l1){
						l2.writeReq(victim->addr);
					}
					if(victim->dirty){
						//write back to memory
					}
					l2.invalidate(victim->addr);
					l2.fillData(addr, victim->way);
				}
				victim = l1.selectVictim(addr);
				if(!victim->valid){
					l1.fillData(addr, victim->way);
				}
				else{
					if(victim->dirty){
						l2.writeReq(victim->addr);
					}
					l1.invalidate(victim->addr);
					l1.fillData(addr, victim->way);
				}
				l1.writeReq(addr);
			}
		}
		else
		{
			if(alloc)
			{
				Block* victim = l1.selectVictim(addr);
				if(!victim->valid){
					l1.fillData(addr, victim->way);
				}
				else{
					if(victim->dirty){
						l2.writeReq(victim->addr);
					}
					l1.invalidate(victim->addr);
					l1.fillData(addr, victim->way);
				}
				l1.writeReq(addr);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	CacheSim simulator (MemCyc, BSize, L1Size, L2Size, L1Assoc,
			L2Assoc, L1Cyc, L2Cyc, WrAlloc);

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		// DEBUG - remove this line
		cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		// DEBUG - remove this line
		cout << " (dec) " << num << endl;

		if(operation == 'r')
		{
			simulator.read(num);
		}
		if(operation == 'w')
		{
			simulator.write(num);
		}

	}

	double L1MissRate = simulator.getL1MissRate();
	double L2MissRate = simulator.getL1MissRate();
	double avgAccTime = simulator.avgAccTime();;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
