package main

import (
	"encoding/csv"
	"fmt"

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

type Packet struct {
	*messages.Packet
	CpuID uint64
}

func SimulateConcurrent(inputs map[int]reader.PBReader, outFile *csv.Writer) *Stats {
	stateChannel := make(chan map[uint64]*cachestate.State)
	accessChannel := make(chan []*Packet)
	cacheChangeChan := make(chan []*CacheChange)
	finishedChan := make(chan FinishedMessage)
	statsChan := make(chan *Stats)

	go worker(0, stateChannel, accessChannel, cacheChangeChan, finishedChan, statsChan)

	stats := coordinator(inputs, stateChannel, accessChannel, cacheChangeChan, finishedChan)
	return stats
}

func coordinator(inputReaders map[int]reader.PBReader, stateChan chan map[uint64]*cachestate.State, accessChan chan []*Packet, cacheChangeChan chan []*CacheChange, finishedChan chan FinishedMessage) *Stats {
	states := make(map[uint64]*cachestate.State)
	for i := uint64(0); i < 5; i++ {
		states[i] = cachestate.New(cacheSize)
	}
outer:
	for true {
		//Push accesses
		packets := []*Packet{}
		for i := 0; i < 10; i++ {
			input, found := inputReaders[0]
			if !found {
				break outer
			}
			packet, err := input.ReadPacket()
			if err != nil {
				panic(err)
			} else if packet == nil {
				fmt.Println("Simulated all packets")
				return &Stats{}
			}
			packets = append(packets, &Packet{Packet: packet, CpuID: 0})

		}

		stateChan <- states
		accessChan <- packets
		//Wait for accesses to be processed

		cacheChanges := <-cacheChangeChan
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

//TODO maybe refactor worker to a struct with a "doWork" function
func worker(workerID int, stateChan chan map[uint64]*cachestate.State, accessChan chan []*Packet, cacheChangeChan chan []*CacheChange, finishedChan chan FinishedMessage, statsChan chan *Stats) {
	stats := Stats{}
	//TODO a way to exit this loop
	for true {
		states := <-stateChan
		accesses := <-accessChan
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
		cacheChangeChan <- cacheChanges
		finishedChan <- FinishedMessage{workerID: workerID, stats: &stats}
	}
}
