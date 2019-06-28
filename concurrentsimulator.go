package main

import (
	"encoding/csv"
	"fmt"
	"math/rand"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

type MemoryChange struct {
	evict     uint64
	entry     uint64
	timestamp uint64
	cpuID     uint64
}

type FinishedMessage struct {
	workerID int
	error    error
}

type Packet struct {
	*messages.Packet
	CpuID uint64
}

func SimulateConcurrent(inputs map[int]reader.PBReader, outFile *csv.Writer) *Stats {
	stats := &Stats{
		MemoryReads:    0,
		MemoryWrites:   0,
		CacheWrites:    0,
		CacheMisses:    0,
		CacheHits:      0,
		CacheEvictions: 0,
	}

	stateChannel := make(chan map[uint64]*cachestate.State)
	accessChannel := make(chan []*Packet)
	memChangeChan := make(chan *MemoryChange)
	finishedChan := make(chan FinishedMessage)

	go worker(0, stateChannel, accessChannel, memChangeChan, finishedChan)

	coordinator(inputs, stateChannel, accessChannel, memChangeChan, finishedChan)

	return stats
}

func coordinator(inputReaders map[int]reader.PBReader, stateChan chan map[uint64]*cachestate.State, accessChan chan []*Packet, memChangeChan chan *MemoryChange, finishedChan chan FinishedMessage) {
	states := make(map[uint64]*cachestate.State)
	for i := uint64(0); i < 5; i++ {
		states[i] = cachestate.New()
	}
	//Push accesses
	packets := []*Packet{}
	for i := 0; i < 10; i++ {
		input, found := inputReaders[0]
		if !found {
			fmt.Println("out of accesses", i)
			break
		}
		packet, err := input.ReadPacket()
		if err != nil {
			panic(err)
		}
		packets = append(packets, &Packet{Packet: packet, CpuID: uint64(rand.Int() % 5)})

	}
	stateChan <- states
	accessChan <- packets
	//Wait for accesses to be processed
outer:
	for true {
		select {
		case finished := <-finishedChan:
			fmt.Printf("Worker %d with error: %v\n", finished.workerID, finished.error)
			break outer
		case memChange := <-memChangeChan:
			fmt.Println(memChange)
			//TODO update state using this memchange
		}
	}

}

//TODO maybe refactor worker to a struct with a "doWork" function
func worker(workerID int, stateChan chan map[uint64]*cachestate.State, accessChan chan []*Packet, memChangeChan chan *MemoryChange, finishedChan chan FinishedMessage) {
	//TODO a way to exit this loop
	for true {
		states := <-stateChan
		accesses := <-accessChan
		for _, access := range accesses {
			state, found := states[access.CpuID]
			if !found {
				finishedChan <- FinishedMessage{workerID: workerID, error: fmt.Errorf("State for given cpuID not found")}
			}
			cacheLine := access.GetAddr() - (access.GetAddr() % cacheLineSize)
			if !state.Contains(cacheLine) {
				memChangeChan <- &MemoryChange{evict: cacheLine, entry: cacheLine, cpuID: access.CpuID} //TODO calculate eviction

				//Search other caches
				foundInOtherCPU := false
				for _, state := range states {
					foundInOtherCPU = foundInOtherCPU || state.Contains(cacheLine) //TODO benchark this vs if
				}
				if !foundInOtherCPU {
					//TODO do what we need to do in this case
				}
			} else {
				memChangeChan <- &MemoryChange{evict: cacheLine, entry: cacheLine, cpuID: access.CpuID} //TODO cpuid
			}
		}
		finishedChan <- FinishedMessage{workerID: workerID}
	}
}
