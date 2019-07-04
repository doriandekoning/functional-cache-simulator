package main

import (
	"encoding/csv"
	"fmt"
	"strconv"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

func simulateSequential(input reader.PBReader, outFile *csv.Writer) *Stats {
	//TODO cachesets

	//Setup states
	states := make([]*cachestate.State, numCpus)
	for i := range states {
		states[i] = cachestate.New(cacheSize)
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
		currentState := states[packet.GetCpuID()].GetState(cacheLine)
		newState, busReq := cachestate.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err = states[packet.GetCpuID()].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range states {
			if uint64(i) != packet.GetCpuID() {
				oldState := states[i].GetState(cacheLine)
				if oldState != cachestate.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cachestate.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}) //Writeback
						if err != nil {
							panic(err)
						}
					}
					err := states[i].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
					if err != nil {
						panic(err)
					}
				}
			}

		}
		if busReq == cachestate.BUS_READ && !foundInOtherCpu {
			err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "r"})
			if err != nil {
				panic(err)
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
