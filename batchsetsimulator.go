package main

import (
	"encoding/csv"
	"fmt"
	"strconv"
	"sync"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

func SetsBatch(inChannel chan *messages.Packet, outFile *csv.Writer, batchSize int) *Stats {

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
	outChannel := make(chan []string, 1000)
	go WriteThread(outChannel)
	var wg sync.WaitGroup
	for i := range channels {
		wg.Add(1)
		go processAccessesBatch(&wg, channels[i], i, states[i], outChannel)

	}
	wg.Wait()
	outWriter.Flush()
	return &Stats{}
}

func processAccessesBatch(wg *sync.WaitGroup, input chan *messages.Packet, cacheSetId int, states []*cacheset.State, out chan []string) {
	// changeChannels := make([]chan *cacheset.CacheStateChange, 2*batchSize)
	for true {
		packet := <-input
		if packet == nil {
			fmt.Println("Finished")
			wg.Done()
			break
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		currentState := states[packet.GetCpuID()].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err := states[packet.GetCpuID()].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState, Timestamp: packet.GetTick()})
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
						out <- []string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}
					}
					err := states[i].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState, Timestamp: packet.GetTick()})
					if err != nil {
						panic(err)
					}
				}
			}

		}
		if busReq == cacheset.BUS_READ && !foundInOtherCpu {
			out <- []string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "r"}
		}

		for i := range states {
			for states[i].GetInUse() > cacheSize {
				err := states[i].ApplyStateChange(&cacheset.CacheStateChange{Address: states[i].GetLRU(), NewState: cacheset.STATE_INVALID, Timestamp: packet.GetTick()})
				if err != nil {
					panic(err)
				}
			}
		}
	}
}
