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
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		currentState := states[packet.GetCpuID()][packet.GetAddr()%numCacheSets].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err = states[packet.GetCpuID()][packet.GetAddr()%numCacheSets].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range states {
			if uint64(i) != packet.GetCpuID() {
				oldState := states[i][packet.GetAddr()%numCacheSets].GetState(cacheLine)
				if oldState != cacheset.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cacheset.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}) //Writeback
						if err != nil {
							panic(err)
						}
					}
					err := states[i][packet.GetAddr()%numCacheSets].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
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
			for states[i][packet.GetAddr()%numCacheSets].GetInUse() > cacheSize {
				err := states[i][packet.GetAddr()%numCacheSets].ApplyStateChange(&StateChange{address: states[i][packet.GetAddr()%numCacheSets].GetLRU(), newState: cacheset.STATE_INVALID})
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
