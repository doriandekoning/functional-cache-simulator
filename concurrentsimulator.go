package main

import (
	"encoding/csv"
	"fmt"
	"time"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
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

func SimulateConcurrent(inputs map[int]reader.PBReader, outFile *csv.Writer, numThreads int, batchSize int) *Stats {
	fmt.Println("Using:", numThreads, "threads")
	finishedChanel := make(chan FinishedMessage, 3)

	stats := coordinator(inputs, finishedChanel, numThreads, batchSize)
	return stats
}

func coordinator(inputReaders map[int]reader.PBReader, finishedChan chan FinishedMessage, numThreads int, batchSize int) *Stats {
	cacheChangeChannel := make(chan []*CacheChange, numThreads*batchSize)

	states := make(map[uint64]*cachestate.State)
	for i := uint64(0); i < 8; i++ {
		states[i] = cachestate.New(cacheSize)
	}
	input, found := inputReaders[0]
	if !found {
		panic("Input reader not found")
	}
	timeSpentSeq := time.Duration(0)
	timeSpentParallel := time.Duration(0)
	counter := 0
	for true {
		counter++
		if counter%100000 == 0 {
			fmt.Println("Processed:", counter, "batches")
		}
		startP1 := time.Now()
		for i := 0; i < numThreads; i++ {
			//Push accesses
			packets, err := reader.ReadMultiple(input, batchSize) //TODO divide input smarter
			if err != nil {
				panic(err)
			}
			if len(packets) == 0 {
				fmt.Println("Reached end of trace file")
				fmt.Println("Sequential:", timeSpentSeq)
				fmt.Println("Parallel:", timeSpentParallel)
				return &Stats{}
			}
			go worker(0, states, packets, cacheChangeChannel)
		}
		timeSpentSeq += time.Now().Sub(startP1)
		startP2 := time.Now()
		//Wait for accesses to be processed
		cacheChanges := []*CacheChange{}
		for i := 0; i < numThreads; i++ {
			changes := <-cacheChangeChannel
			cacheChanges = append(cacheChanges, changes...)
		}
		timeSpentParallel += time.Now().Sub(startP2)
		startP3 := time.Now()
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
		timeSpentSeq += time.Now().Sub(startP3)

	}

	return &Stats{}
}

func worker(workerID int, states map[uint64]*cachestate.State, accesses []*messages.Packet, changeChan chan []*CacheChange) {
	stats := Stats{}
	cacheChanges := []*CacheChange{}
	for _, access := range accesses {
		state, found := states[access.GetCpuID()]
		if !found {
			stats.CacheMisses++
			return
		}
		cacheLine := access.GetAddr() - (access.GetAddr() % cacheLineSize)
		//Handle a cache miss
		if !state.Contains(cacheLine) {
			cacheChanges = append(cacheChanges, &CacheChange{changeType: cachestate.ChangeTypeEvict, put: cacheLine, cpuID: access.GetCpuID()})

			//Search other caches
			foundInOtherCPU := false
			for _, state := range states {
				foundInOtherCPU = foundInOtherCPU || state.Contains(cacheLine)
				if foundInOtherCPU {
					break
				}
			}
			if !foundInOtherCPU {
				stats.MemoryReads++
				cacheChanges = append(cacheChanges, &CacheChange{put: cacheLine, cpuID: access.GetCpuID()})
			}
			//Handle a cache hit
		} else {
			stats.CacheHits++
			cacheChanges = append(cacheChanges, &CacheChange{changeType: cachestate.ChangeTypeMoveFront, put: cacheLine, cpuID: access.GetCpuID()})
		}
	}
	changeChan <- cacheChanges
}
