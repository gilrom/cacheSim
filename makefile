# 046267 Computer Architecture - Winter 20/21 - HW #2

cacheSim: cacheSim.cpp
	g++ -o cacheSim cacheSim.cpp cache_sim.h

.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
