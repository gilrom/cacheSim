/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <stdlib>
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

Cache::Cache(uint size, uint associativ, uint cycle, uint block_size) : size(size),
 num_of_calls(0), num_of_miss(0), assoc(associativ), acc_cyc(cycle), table(nullptr)
{
	
	num_of_set_bits = size - (block_size + assoc);
	num_of_tag_bits = 32 -2 -block_size -num_of_set_bits;

	uint num_of_Lines = 1 << num_of_set_bits;
	uint num_of_Ways = 1 << assoc;

	table = new Block*[num_of_Lines]; 
	for(int i = 0 ; i < num_of_Lines ; i++)
	{
		table[i] = new Block[num_of_Ways];
		for(int j = 0 ; j < num_of_Ways ; j++)
		{
			table[i][j].dirty = false;
			table[i][j].valid = false;
			table[i][j].lru_key = j;
			table[i][j].way = j;
			table[i][j].addr = 0;
		}
	}
}

Cache::~Cache(){
	for (int i = 0; i < (1 << num_of_set_bits); i++)
	{
		delete[] table[i];
	}
	delete[] table;
}

void Cache::parseSetAndTag(uint addr, uint* tag, uint* set){
	*tag = addr >> (32 - num_of_tag_bits);
	*set = (addr << num_of_tag_bits) >> (32 - num_of_set_bits);
}

void Cache::updateLru(uint index_of_adrr, uint set){
	uint x = table[set][index_of_adrr].lru_key;
	table[set][index_of_adrr].lru_key = (1 << assoc) - 1;
	for (int i = 0; i < (1 << assoc); i++)
	{
		if(i != index_of_adrr && table[set][i].lru_key > x)
			table[set][i].lru_key--;
	}
}

void Cache::invalidate (uint32_t addr)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	for (int i = 0; i < (1 << assoc); i++)
	{
		if(!table[set][i].valid)
		{
			continue;
		}
		if(table[set][i].tag == tag)
		{
			table[set][i].valid = false;	
			return;
		}
	}
	
}

bool Cache::snoop(uint32_t addr)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	for (int i = 0; i < (1 << assoc); i++)
	{
		if(!table[set][i].valid)
		{
			continue;
		}
		if(table[set][i].tag == tag)
		{
			table[set][i].valid = false;	
			return table[set][i].dirty;
		}
	}
	return false;
}

bool Cache::writeReq(uint32_t addr, bool realReq = false)
{
	if (realReq) num_of_calls++;

	uint tag, set;
	parseSetAndTag(addr, &tag, &set);

	for (int i = 0; i < (1 << assoc); i++)
	{
		if(!table[set][i].valid)
		{
			continue;
		}
		if(table[set][i].tag == tag)
		{
			table[set][i].dirty = true;	
			if (realReq) updateLru(i,set);
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

	for (int i = 0; i < (1 << assoc); i++)
	{
		if(!table[set][i].valid)
		{
			continue;
		}
		if(table[set][i].tag == tag)
		{
			updateLru(i,set);
			return true;
		}
	}
	num_of_miss++;
	return false;
}

//on work
Block& Cache::selectVictim(uint32_t addr)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);
	uint min_lru_key = 1 << assoc;
	int index = 0;

	for (int i = 0; i < (1 << assoc); i++)
	{
		if(table[set][i].valid)
		{
			if(min_lru_key > table[set][i].lru_key)
			{
				min_lru_key = table[set][i].lru_key;
				index = i;
			}
		}
		else
		{
			return table[set][i];
		}
	}
	return table[set][index];
}

void Cache::fillData(uint32_t addr, int way)
{
	uint tag, set;
	parseSetAndTag(addr, &tag, &set);
	table[set][way].tag = tag;
	table[set][way].valid = true;
	table[set][way].dirty = false;
	table[set][way].addr = addr;
	updateLru(addr, set);
}

double Cache::getMissRate()
{
	return (double)num_of_miss/(double)num_of_calls;
}

uint Cache::getNumOfAcc();

CacheSim::CacheSim(uint MemCyc, uint BSize, uint L1Size, uint L2Size, uint L1Assoc,
			uint L2Assoc, uint L1Cyc, uint L2Cyc, uint WrAlloc) :

	alloc(WrAlloc), block_size(BSize), mem_cyc(MemCyc), num_of_mem_acc(0), 
	L1Cyc(L1Cyc), L2Cyc(L2Cyc),
	l1(L1Size, L1Assoc, BSize),	l2(L2Size, L2Assoc, BSize)
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
	return = total_time/total_calls;
}

void CacheSim::read(uint32_t addr){
	if(!l1.readReq(addr)){
		if(!l2.readReq(addr)){
			num_of_mem_acc++;
			Block& b = l2.selectVictim(addr);
			if(!b.valid){
				l2.fillData(addr, b.way);
			}
			else{
				bool dirty_in_l1 = l1.snoop(b.addr);
				if(dirty_in_l1){
					l2.writeReq(b.addr);
				}
				if(b.dirty){
					//write back to memory
				}
				l2.invalidate(b.addr);
				l2.fillData(addr, b.way);
			}
			Block& b = l1.selectVictim(addr);
			if(!b.valid){
				l1.fillData(addr, b.way);
			}
			else{
				if(b.dirty){
					l2.writeReq(b.addr);
				}
				l1.invalidate(b.addr);
				l1.fillData(addr, b.way);
			}
		}
		else{
			Block& b = l1.selectVictim(addr);
			if(!b.valid){
				l1.fillData(addr, b.way);
			}
			else{
				if(b.dirty){
					l2.writeReq(b.addr);
				}
				l1.invalidate(b.addr);
				l1.fillData(addr, b.way);
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

		if(operation == 'R')
		{
			simulator.read(num);
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
