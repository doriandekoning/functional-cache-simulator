package main

// import (
// 	"encoding/csv"
// 	"fmt"
// 	"io"

// 	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
// 	"github.com/doriandekoning/functional-cache-simulator/pkg/cachestate"
// 	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
// )


// func simulateBatch(inputs map[int]reader.PBReader, outFile *csv.Writer) *Stats {
// 	stats := &Stats{
// 		MemoryReads:    0,
// 		MemoryWrites:   0,
// 		CacheWrites:    0,
// 		CacheMisses:    0,
// 		CacheHits:      0,
// 		CacheEvictions: 0,
// 	}
// 	states := make(map[int]*cachestate.State)
// 	for i, _ := range inputs {
// 		states[i] = cachestate.New(64)//TODO variable
// 	}

// 	writeBack := 0
// 	//TODO:flushing

// 	for true {
// 		packet, id, err := getNextPacket(inputs)
// 		if err == io.EOF {
// 			fmt.Println("EOF reached")
// 			break
// 		} else if err != nil {
// 			panic(err)
// 		}
// 		if packet == nil {
// 			break
// 		}
// 		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
// 		item := states[id].addressMap[cacheLine]
// 		write := packet.GetCmd() != 1
// 		if write {
// 			if item == nil {
// 				stats.LocalCacheMisses++
// 				//PrWrI: accessing processor to M others to I
// 				states[id].addressMap[cacheLine] = &AddressMapEntry{state: STATE_MODIFIED, listItem: states[id].lruList.PushFront(cacheLine)}
// 				found := false
// 				for i, state := range states {
// 					//Update all but the accessing processor to I
// 					if state.addressMap != nil {
// 						found = true
// 					}
// 					if i != id {
// 						delete(state.addressMap, cacheLine)
// 					}
// 				}
// 				if !found {
// 					stats.MemoryWrites++
// 				}
// 			} else if item != nil && item.state == STATE_MODIFIED {
// 				//PrWrM: accessing processor to M others stays same
// 				item.state = STATE_MODIFIED
// 			} else if item != nil && item.state == STATE_SHARED {
// 				//PrWrS: that processor to M others to I
// 				item.state = STATE_MODIFIED
// 				//TODO: fix duplicate code
// 				for i := range states {
// 					//Update all but the accessing processor to I
// 					if i != id {
// 						delete(states[i].addressMap, cacheLine)
// 					}
// 				}
// 			} else {
// 				panic("Dont know what to do")
// 			}

// 		} else {

// 			if item == nil {

// 				stats.LocalCacheMisses++
// 				found := false
// 				//PrRdI: accessing processor to S others to S
// 				for _, state := range states {
// 					if state.addressMap[cacheLine] != nil {
// 						found = true
// 						state.addressMap[cacheLine].state = STATE_SHARED
// 					}
// 				}
// 				if !found {
// 					stats.MemoryReads++
// 				}
// 				states[id].addressMap[cacheLine] = &AddressMapEntry{state: STATE_MODIFIED, listItem: states[id].lruList.PushFront(cacheLine)}

// 			} else if item != nil && item.state == STATE_MODIFIED {
// 				//PrRdM: nothing
// 			} else if item != nil && item.state == STATE_SHARED {
// 				//PrRdS: nothing
// 			} else {
// 				panic("Dont know what to do")
// 			}
// 		}

// 		if states[id].addressMap[cacheLine] != nil {
// 			states[id].lruList.MoveToFront(states[id].addressMap[cacheLine].listItem)
// 		}

// 		if len(states[id].addressMap) > cacheSize {
// 			evictedLine := states[id].lruList.Remove(states[id].lruList.Back()).(uint64)
// 			if states[id].addressMap[evictedLine].state == STATE_MODIFIED {
// 				writeBack++
// 				stats.MemoryWrites++
// 			}
// 			delete(states[id].addressMap, evictedLine)
// 			stats.CacheEvictions++
// 		}
// 	}

// 	return stats
// }

// //getNextPacket gets the next packet with the lowest timestamp and returns it along with it's cpu id
// func getNextPacket(inputs map[int]reader.PBReader) (*messages.Packet, int, error) {
// 	cpuMinTick := inputs[0].NextTick() //TODO:maxint
// 	cpuMinId := 0
// 	for i, input := range inputs {
// 		if nextTick := input.NextTick(); nextTick < cpuMinTick {
// 			cpuMinId = i
// 			cpuMinTick = nextTick
// 		}
// 	}
// 	packet, err := inputs[cpuMinId].ReadPacket() //TODO:catch EOF and remove input
// 	return packet, cpuMinId, err
// }
