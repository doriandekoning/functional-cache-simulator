package main

import (
	"encoding/csv"
	"fmt"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

type CacheChange struct {
	changeType cachestate.ChangeType
	put        uint64
	timestamp  uint64
	cpuID      uint64
}

func (change *CacheChange) GetChangeType() cachestate.ChangeType {
	return change.changeType
}

func (change *CacheChange) GetPut() uint64 {
	return change.put
}

type FinishedMessage struct {
	workerID int
	stats    *Stats
	error    error
}

func SimulateConcurrent(inputs map[int]reader.PBReader, outFile *csv.Writer) *Stats {
	finishedChanel := make(chan FinishedMessage)

	stats := coordinator(inputs, finishedChanel)
	return stats
}

func coordinator(inputReaders map[int]reader.PBReader, finishedChan chan FinishedMessage) *Stats {
	statsChannel := make(chan *Stats)
	cacheChangeChannel := make(chan []*CacheChange)

	states := make(map[uint64]*cachestate.State)
	for i := uint64(0); i < 5; i++ {
		states[i] = cachestate.New(cacheSize)
	}
	input, found := inputReaders[0]
	if !found {
		panic("Input reader not found")
	}
	for true {
		//Push accesses
		packets, err := reader.ReadMultiple(input, 10)
		if err != nil {
			panic(err)
		}
		if len(packets) == 0 {
			fmt.Println("Reached end of trace file")
			return &Stats{}
		}

		go worker(0, states, packets, cacheChangeChannel, finishedChan, statsChannel)

		//Wait for accesses to be processed

		cacheChanges := <-cacheChangeChannel
		for _, cacheChange := range cacheChanges { //TODO parallelize for each state
			state, found := states[cacheChange.cpuID]
			if !found {
				panic("Cannot find state for given cpuid")
			}
			err := state.ApplyStateChange(cacheChange)
			if err != nil {
				panic(err)
			}
		}
		<-finishedChan //TODO gather stats
	}
	return &Stats{}
}

func worker(workerID int, states map[uint64]*cachestate.State, accesses []*reader.Packet, changeChan chan []*CacheChange, finishedChan chan FinishedMessage, statsChannel chan *Stats) {
	stats := Stats{}
	//TODO a way to exit this loop
	// for true {
	// states := <-stateChan
	// accesses := <-accessChan
	cacheChanges := []*CacheChange{} //TODO make list
	for _, access := range accesses {
		state, found := states[access.CpuID]
		if !found {
			stats.CacheMisses++

			finishedChan <- FinishedMessage{workerID: workerID, error: fmt.Errorf("State for given cpuID not found"), stats: &stats}
		}
		cacheLine := access.GetAddr() - (access.GetAddr() % cacheLineSize)
		//Handle a cache miss
		if !state.Contains(cacheLine) {
			cacheChanges = append(cacheChanges, &CacheChange{changeType: cachestate.ChangeTypeEvict, put: cacheLine, cpuID: access.CpuID})

			//Search other caches
			// foundInOtherCPU := false
			// for _, state := range states {
			// 	foundInOtherCPU = foundInOtherCPU || state.Contains(cacheLine) //TODO benchark this vs if
			// }
			// if !foundInOtherCPU {
			// 	stats.MemoryReads++
			// 	// cacheChangeChan <- &CacheChange{put: cacheLine, cpuID: access.CpuID}
			// }
			//Handle a cache hit
		} else {
			stats.CacheHits++
			cacheChanges = append(cacheChanges, &CacheChange{changeType: cachestate.ChangeTypeMoveFront, put: cacheLine, cpuID: access.CpuID})
		}
	}
	changeChan <- cacheChanges
	finishedChan <- FinishedMessage{workerID: workerID, stats: &stats}
}
