#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <vector>

typedef unsigned int uint;


class Block{
        int lru_key;
        public:
        bool valid, dirty;
        uint32_t addr; //search will be by tag bits only
        uint32_t tag; //so we must saves tose tags add cot if needed
        uint way; //block way int the set (index)
        int getLruKey()
        {
            return lru_key;
        }
        void setLruKey(int lru)
        {
            lru_key = lru;
        }
        Block() : valid(false), dirty(false), lru_key(0), addr(0), tag(0), way(0) {}
};

class Cache{
    uint size;
    public:
        uint num_of_calls;
        uint num_of_miss;
    private:
    uint assoc;
    std::vector<Block*> *memory;
    uint num_of_tag_bits;
    uint num_of_set_bits;

    private:
    void updateLru(uint index_of_adrr, uint set);
    void parseSetAndTag(uint32_t addr, uint* tag, uint* set);
    public:
    Cache(uint size, uint associativ, uint block_size);
    ~Cache();
    void invalidate (uint32_t addr);
    bool snoop(uint32_t addr);
    bool writeReq(uint32_t addr, bool realReq = false);
    bool readReq(uint32_t addr); //if founded it update the LRU key //it updates the num_of_calls and num_of_miss
    Block* selectVictim(uint32_t addr);//this will returns null if there is a free space
    void fillData(uint32_t addr, uint ind);
    double getMissRate();
    uint getNumOfAcc();
    void printCache();
};

class CacheSim{
    bool alloc;
    uint block_size;
    uint mem_cyc;
    uint num_of_mem_acc;
    uint L1Cyc;
    uint L2Cyc;

    Cache l1;
    Cache l2;

    public:
        CacheSim(uint MemCyc, uint BSize, uint L1Size, uint L2Size, uint L1Assoc,
			uint L2Assoc, uint L1Cyc, uint L2Cyc, uint WrAlloc);
        double getL1MissRate();
        double getL2MissRate();
        float avgAccTime();
        void read(uint32_t addr);
        void write(uint32_t addr);
};