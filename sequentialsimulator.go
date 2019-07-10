package main

import (
	"encoding/csv"
	"fmt"
	"strconv"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

func simulateSequential(input reader.PBReader, outFile *csv.Writer) *Stats {
	amountOfCacheSets := cacheSize / associativity
	//Setup states
	cacheSets := make([][]*cacheset.State, amountOfCacheSets)
	for i := range cacheSets {
		cacheSets[i] = make([]*cacheset.State, numCpus)
		for j := range cacheSets[i] {
			cacheSets[i][j] = cacheset.New(cacheSize)
		}
	}
	var packetsProcessed uint64
	for true {
		packet, err := input.ReadPacket()
		if packet == nil {
			fmt.Println("Finished")
			break
		} else if err != nil {
			panic(err)
		} else if packet == nil {
			break
		}
		// ( ... >> 6) strips the 64 bits indicating the offset within the cache line
		cacheLine := packet.GetAddr() >> 6
		cacheSetNumber := getCacheSetNumber(cacheLine)
		currentState := cacheSets[cacheSetNumber][packet.GetCpuID()].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err = cacheSets[cacheSetNumber][packet.GetCpuID()].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range cacheSets[cacheSetNumber] {
			if uint64(i) != packet.GetCpuID() {
				oldState := cacheSets[cacheSetNumber][i].GetState(cacheLine)
				if oldState != cacheset.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cacheset.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}) //Writeback
						if err != nil {
							panic(err)
						}
					}
					err := cacheSets[cacheSetNumber][i].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
					if err != nil {
						panic(err)
					}
				}
			}

		}
		if busReq == cacheset.BUS_READ && !foundInOtherCpu {
			err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "r"})
			if err != nil {
				panic(err)
			}
		}

		for i := range cacheSets[cacheSetNumber] {
			for cacheSets[cacheSetNumber][i].GetInUse() > cacheSize {
				err := cacheSets[cacheSetNumber][i].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheSets[cacheSetNumber][i].GetLRU(), NewState: cacheset.STATE_INVALID}) //TODO writeback upon evict if not in other caches (only if in M)
				if err != nil {
					panic(err)
				}
			}
		}

		packetsProcessed++
		if packetsProcessed%500000 == 0 {
			fmt.Println("Processed:", packetsProcessed, "packets")
		}

	}
	outWriter.Flush()
	return &Stats{}
}

func getCacheSetNumber(cacheLine uint64) int {
	// (2<<16) = cachesize in lines
	return int(cacheLine % (2 << 13))
}
