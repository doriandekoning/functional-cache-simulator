package main

import (
	"encoding/csv"
	"fmt"
	"sync"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

func SimulateParallelSet(inChannel chan *messages.Packet, outFile *csv.Writer) *Stats {
	//TODO cachesets

	//Setup states
	states := make([][]*cacheset.State, numCacheSets)
	for i := range states {
		states[i] = make([]*cacheset.State, numCpus)
		for j := 0; j < numCacheSets; j++ {
			states[i][j] = cacheset.New(cacheSize)
		}
	}
	//Setup channels
	channels := make([]chan *messages.Packet, numCacheSets)
	for i := range channels {
		channels[i] = make(chan *messages.Packet, 100)
	}
	go SplitChannels(inChannel, channels)
	var wg sync.WaitGroup
	for i := range channels {
		wg.Add(1)
		go processAccesses(&wg, channels[i], i, states[i])

	}
	wg.Wait()
	outWriter.Flush()
	return &Stats{}
}

func processAccesses(wg *sync.WaitGroup, input chan *messages.Packet, cacheSetId int, states []*cacheset.State) {
	for true {
		packet := <-input
		if packet == nil { //TODO push nil when finished
			fmt.Println("Finished")
			wg.Done()
			break
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		currentState := states[packet.GetCpuID()].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err := states[packet.GetCpuID()].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range states {
			if uint64(i) != packet.GetCpuID() {
				oldState := states[i].GetState(cacheLine)
				if oldState != cacheset.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cacheset.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						// err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}) //Writeback //TODO add out channel
						// if err != nil {
						// 	panic(err)
						// }
					}
					err := states[i].ApplyStateChange(&StateChange{address: cacheLine, newState: newState})
					if err != nil {
						panic(err)
					}
				}
			}

		}
		if busReq == cacheset.BUS_READ && !foundInOtherCpu {
			// err := outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "r"})
			// if err != nil {
			// 	panic(err)
			// }
		}

		for i := range states {
			for states[i].GetInUse() > cacheSize {
				err := states[i].ApplyStateChange(&StateChange{address: states[i].GetLRU(), newState: cacheset.STATE_INVALID})
				if err != nil {
					panic(err)
				}
			}
		}
	}
}

func SplitChannels(input chan *messages.Packet, out []chan *messages.Packet) {
	for true {
		new := <-input
		if new == nil {
			for i := range out {
				out[i] <- nil
			}
		} else {
			out[new.GetAddr()%numCacheSets] <- new
		}
	}
}
