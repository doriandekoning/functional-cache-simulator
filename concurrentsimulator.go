package main

import (
	"encoding/csv"
	"fmt"
	"time"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

var cacheSets = 1
var numCpus = 8

type StateChange struct {
	newState cachestate.CacheLineState
	address  uint64
}

func (s *StateChange) GetNewState() cachestate.CacheLineState {
	return s.newState
}

func (s *StateChange) GetAddress() uint64 {
	return s.address
}

func SimulateConcurrent(inputChannel chan *messages.Packet, outFile *csv.Writer, numThreads int, batchSize int) *Stats {
	fmt.Println("Using:", numThreads, "threads")

	//Initialize updateachannels
	updateChannels := make([][]chan cachestate.CacheStateChange, cacheSets)
	for i := 0; i < cacheSets; i++ {
		updateChannels[i] = make([]chan cachestate.CacheStateChange, numCpus)
		for j := 0; j < numCpus; j++ {
			updateChannels[i][j] = make(chan cachestate.CacheStateChange, 10) //TODO make size dynamic
		}
	}
	//Initialize stateChannels
	stateChannels := make([]chan *[][]cachestate.State, numThreads)
	for i := 0; i < numThreads; i++ {
		stateChannels[i] = make(chan *[][]cachestate.State, 10)
	}
	//Setup accessWorker threads
	for i := 0; i < numThreads; i++ {
		go accessWorker(inputChannel, stateChannels[i], updateChannels)
	}

	states := make([][]cachestate.State, cacheSets)
	for true {
		for i := 0; i < cacheSets; i++ {
			states[i] = make([]cachestate.State, numCpus)
			for j := 0; j < numCpus; j++ {
				states[i][j] = *cachestate.New(100)
				retChannel := make(chan *cachestate.State)
				stateUpdateWorker(updateChannels[i][j], states[i][j].Copy(), retChannel)
				states[i][j] = *<-retChannel
			}
		}
		time.Sleep(5 * time.Second)
		for i := 0; i < numThreads; i++ {
			stateChannels[i] <- &states
		}
	}

	return &Stats{}
	// finishedChanel := make(chan FinishedMessage, 3)

}

//Update chans is a map where the first index specifies the cache set and the second the cache
func accessWorker(input chan *messages.Packet, stateChan chan *[][]cachestate.State, updateChans [][]chan cachestate.CacheStateChange) {
	var states *[][]cachestate.State
	i := 0
	for true {
		if i%100 == 0 {
			states = <-stateChan
		}
		i++
		curAccess := <-input
		write := curAccess.GetCmd() != 1
		cacheSet := determineCacheSet(curAccess.GetAddr())
		curState := (*states)[cacheSet][curAccess.GetCpuID()].GetState(curAccess.GetAddr())
		if write {
			if curState == cachestate.STATE_INVALID || curState == cachestate.STATE_SHARED {
				//PrWrI: accessing processor to M others to I
				//PrWrS: that processor to M others to I
				for i := range (*states)[cacheSet] {
					stateChange := StateChange{
						address:  curAccess.GetAddr(),
						newState: cachestate.STATE_MODIFIED,
					}
					if i == int(curAccess.GetCpuID()) {
						stateChange.newState = cachestate.STATE_MODIFIED
					}
					updateChans[cacheSet][i] <- &stateChange
				}
			} else if curState == cachestate.STATE_MODIFIED {
				//PrWrM: accessing processor to M others stays same
				stateChange := StateChange{
					address:  curAccess.GetAddr(),
					newState: cachestate.STATE_MODIFIED,
				}
				updateChans[cacheSet][int(curAccess.GetCpuID())] <- &stateChange
			} else {
				panic("Dont know what to do")
			}
		} else {
			if curState == cachestate.STATE_INVALID {
				//PrRdI: accessing processor to S others to S
				for i := range (*states)[cacheSet] {
					stateChange := StateChange{
						address:  curAccess.GetAddr(),
						newState: cachestate.STATE_SHARED,
					}
					updateChans[cacheSet][i] <- &stateChange
				}
			} else if curState == cachestate.STATE_MODIFIED {
				//PrRdM: nothing
			} else if curState == cachestate.STATE_SHARED {
				//PrRdS: nothing
			} else {
				panic("Dont know what to do")
			}
		}
	}
}

func stateUpdateWorker(in chan cachestate.CacheStateChange, newState cachestate.State, retChannel chan *cachestate.State) { //TODO refactor in and out to channels
	for i := 0; i < 100; i++ {
		<-in
		newState.ApplyStateChange(<-in)
	}
	retChannel <- &newState
}

func determineCacheSet(address uint64) int {
	return int(address) % cacheSets //TODO realistic mapping
}
