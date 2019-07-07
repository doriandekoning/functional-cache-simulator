package main

import (
	"encoding/csv"
	"fmt"
	"strconv"
	"sync"
	"time"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

var startTime time.Time

func SimulateParallelSet(inChannel chan *messages.Packet, outFile *csv.Writer) *Stats {
	startTime = time.Now()
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
		channels[i] = make(chan *messages.Packet, 10000)
	}
	go SplitChannels(inChannel, channels)
	outChannel := make(chan []string, 1000)
	go WriteThread(outChannel)
	var wg sync.WaitGroup
	for i := range channels {
		wg.Add(1)
		go processAccesses(&wg, channels[i], i, states[i], outChannel)

	}
	wg.Wait()
	outWriter.Flush()
	return &Stats{}
}

func processAccesses(wg *sync.WaitGroup, input chan *messages.Packet, cacheSetId int, states []*cacheset.State, out chan []string) {
	for true {
		packet := <-input
		if packet == nil {
			fmt.Println("Finished", startTime.Sub(time.Now()))
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

func SplitChannels(input chan *messages.Packet, out []chan *messages.Packet) {
	for true {
		new := <-input
		if new == nil {
			for i := range out {
				out[i] <- nil
			}
		} else {
			out[(new.GetAddr()/cacheLineSize)%numCacheSets] <- new
		}
	}
}

func WriteThread(in chan []string) {
	for {
		err := outWriter.Write(<-in)
		if err != nil {
			panic(err)
		}
	}
}
