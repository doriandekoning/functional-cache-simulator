package main

import (
	"encoding/csv"
	"fmt"
	"strconv"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

func simulateSequential(input reader.PBReader, outFile *csv.Writer) *Stats {
	//TODO cachesets

	//Setup states
	states := make([][]*cacheset.State, numCpus)
	for i := range states {
		states[i] = make([]*cacheset.State, numCacheSets)
		for j := 0; j < numCacheSets; j++ {
			states[i][j] = cacheset.New(cacheSize)
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
		if ((int(packet.GetAddr()) / cacheLineSize) % numCacheSets) != 1 {
			continue
		}

		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		currentState := states[packet.GetCpuID()][getCacheSetNumber(packet.GetAddr())].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err = states[packet.GetCpuID()][getCacheSetNumber(packet.GetAddr())].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range states {
			if uint64(i) != packet.GetCpuID() {
				oldState := states[i][getCacheSetNumber(packet.GetAddr())].GetState(cacheLine)
				if oldState != cacheset.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cacheset.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}) //Writeback
						if err != nil {
							panic(err)
						}
					}
					err := states[i][getCacheSetNumber(packet.GetAddr())].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
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

		for i := range states {
			for states[i][getCacheSetNumber(packet.GetAddr())].GetInUse() > cacheSize {
				err := states[i][getCacheSetNumber(packet.GetAddr())].ApplyStateChange(&cacheset.CacheStateChange{Address: states[i][getCacheSetNumber(packet.GetAddr())].GetLRU(), NewState: cacheset.STATE_INVALID})
				if err != nil {
					panic(err)
				}
			}
		}

		packetsProcessed++
		if packetsProcessed%10000000 == 0 {
			fmt.Println("Processed:", packetsProcessed, "packets")
		}

	}
	outWriter.Flush()
	return &Stats{}
}

func getCacheSetNumber(address uint64) int {
	return (int(address) / cacheLineSize) % numCacheSets
}
