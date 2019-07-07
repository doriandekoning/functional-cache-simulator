package main

import (
	"encoding/csv"
	"fmt"
	"time"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

var cacheSets = 1
var numCpus = 8

func SimulateConcurrent(inputChannel chan *messages.Packet, outFile *csv.Writer, numThreads int, batchSize int) *Stats {
	fmt.Println("Using:", numThreads, "threads")

	//Initialize updateachannels
	updateChannels := make([][]chan *cacheset.CacheStateChange, cacheSets)
	for i := 0; i < cacheSets; i++ {
		updateChannels[i] = make([]chan *cacheset.CacheStateChange, numCpus)
		for j := 0; j < numCpus; j++ {
			updateChannels[i][j] = make(chan *cacheset.CacheStateChange, 10) //TODO make size dynamic
		}
	}
	//Initialize stateChannels
	stateChannels := make([]chan *[][]cacheset.State, numThreads)
	for i := 0; i < numThreads; i++ {
		stateChannels[i] = make(chan *[][]cacheset.State, 10)
	}
	//Setup accessWorker threads
	for i := 0; i < numThreads; i++ {
		go accessWorker(inputChannel, stateChannels[i], updateChannels)
	}

	states := make([][]cacheset.State, cacheSets)
	for true {
		for i := 0; i < cacheSets; i++ {
			states[i] = make([]cacheset.State, numCpus)
			for j := 0; j < numCpus; j++ {
				states[i][j] = *cacheset.New(100)
				retChannel := make(chan *cacheset.State)
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
func accessWorker(input chan *messages.Packet, stateChan chan *[][]cacheset.State, updateChans [][]chan *cacheset.CacheStateChange) {
	var states *[][]cacheset.State
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
			if curState == cacheset.STATE_INVALID || curState == cacheset.STATE_SHARED {
				//PrWrI: accessing processor to M others to I
				//PrWrS: that processor to M others to I
				for i := range (*states)[cacheSet] {
					stateChange := cacheset.CacheStateChange{
						Address:  curAccess.GetAddr(),
						NewState: cacheset.STATE_MODIFIED,
					}
					if i == int(curAccess.GetCpuID()) {
						stateChange.NewState = cacheset.STATE_MODIFIED
					}
					updateChans[cacheSet][i] <- &stateChange
				}
			} else if curState == cacheset.STATE_MODIFIED {
				//PrWrM: accessing processor to M others stays same
				stateChange := cacheset.CacheStateChange{
					Address:  curAccess.GetAddr(),
					NewState: cacheset.STATE_MODIFIED,
				}
				updateChans[cacheSet][int(curAccess.GetCpuID())] <- &stateChange
			} else {
				panic("Dont know what to do")
			}
		} else {
			if curState == cacheset.STATE_INVALID {
				//PrRdI: accessing processor to S others to S
				for i := range (*states)[cacheSet] {
					stateChange := cacheset.CacheStateChange{
						Address:  curAccess.GetAddr(),
						NewState: cacheset.STATE_SHARED,
					}
					updateChans[cacheSet][i] <- &stateChange
				}
			} else if curState == cacheset.STATE_MODIFIED {
				//PrRdM: nothing
			} else if curState == cacheset.STATE_SHARED {
				//PrRdS: nothing
			} else {
				panic("Dont know what to do")
			}
		}
	}
}

func stateUpdateWorker(in chan *cacheset.CacheStateChange, newState cacheset.State, retChannel chan *cacheset.State) { //TODO refactor in and out to channels
	for i := 0; i < 100; i++ {
		<-in
		newState.ApplyStateChange(<-in)
	}
	retChannel <- &newState
}

func determineCacheSet(address uint64) int {
	fmt.Println(int(address), cacheSets, int(address)%cacheSets)
	return int(address) % cacheSets //TODO realistic mapping
}
