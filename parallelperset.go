package main

import (
	"encoding/csv"
	"fmt"
	"strconv"
	"sync"

	"github.com/doriandekoning/functional-cache-simulator/pkg/cacheset"
	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

const channelSize = 10000
const outChannelSize = 100

func simulateParallel(input chan *messages.Packet, outFile *csv.Writer) *Stats {
	//Setup states
	cacheSets := make([][]*cacheset.State, numCacheSets)
	for i := range cacheSets {
		cacheSets[i] = make([]*cacheset.State, numCpus)
		for j := range cacheSets[i] {
			cacheSets[i][j] = cacheset.New(cacheSize)
		}
	}

	var waitgroup sync.WaitGroup
	//Setup channels
	packetChannels := make([]chan *messages.Packet, *threads)
	outChannel := make(chan *[]string, outChannelSize)
	go Writer(outChannel)

	waitgroup.Add(len(packetChannels))
	for i := range packetChannels {
		packetChannels[i] = make(chan *messages.Packet, channelSize)
		go processPackets(packetChannels[i], cacheSets, outChannel, &waitgroup)
	}

	var packetsProcessed uint64
	go func() {
		for true {
			packet := <-input
			if packet == nil || packetsProcessed > *maxAccesses {
				fmt.Println("Finished")
				for i := range packetChannels {
					packetChannels[i] <- nil
				}
				break
			}
			cacheSet := getCacheSetNumber(packet.GetAddr() >> 6)
			packetChannels[cacheSet%*threads] <- packet
			packetsProcessed++
			if packetsProcessed%500000 == 0 {
				fmt.Println("Processed:", packetsProcessed, "packets")
			}
		}
	}()
	waitgroup.Wait()
	outWriter.Flush()
	return &Stats{}
}

func processPackets(input chan *messages.Packet, cacheSets [][]*cacheset.State, outChannel chan *[]string, wg *sync.WaitGroup) {
	var totalPackets int
	for true {
		packet := <-input
		if packet == nil {
			fmt.Println("totalPackets", totalPackets)
			wg.Done()
			return
		}
		totalPackets++
		// ( ... >> 6) strips the 64 bits indicating the offset within the cache line
		cacheLine := packet.GetAddr() >> 6
		cacheSetNumber := getCacheSetNumber(cacheLine)
		currentState := cacheSets[cacheSetNumber][packet.GetCpuID()].GetState(cacheLine)
		newState, busReq := cacheset.GetMSIStateChange(currentState, packet.GetCmd() == 1)
		err := cacheSets[cacheSetNumber][packet.GetCpuID()].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
		if err != nil {
			panic(err)
		}
		var foundInOtherCpu bool
		for i := range cacheSets[cacheSetNumber] {
			if uint64(i) != packet.GetCpuID() {
				oldState := cacheSets[cacheSetNumber][i].GetState(cacheLine)
				if oldState != cacheset.STATE_INVALID {
					foundInOtherCpu = true
					newState, flush := cacheset.GetMSIStateChangeByBusRequest(oldState, busReq)
					if flush {
						outChannel <- &[]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "w"}
					}
					err := cacheSets[cacheSetNumber][i].ApplyStateChange(&cacheset.CacheStateChange{Address: cacheLine, NewState: newState})
					if err != nil {
						panic(err)
					}
				}
			}

		}
		if busReq == cacheset.BUS_READ && !foundInOtherCpu {
			outChannel <- &[]string{strconv.FormatUint(packet.GetAddr(), 10), strconv.FormatUint(packet.GetTick(), 10), "r"}
		}

		for i := range cacheSets[cacheSetNumber] {
			for cacheSets[cacheSetNumber][i].GetInUse() > cacheSize {
				lru := cacheSets[cacheSetNumber][i].GetLRU()
				if cacheSets[cacheSetNumber][i].GetState(lru) == cacheset.STATE_MODIFIED {
					outChannel <- &[]string{strconv.FormatUint(lru, 10), strconv.FormatUint(packet.GetTick(), 10), "w"}
				}
				err := cacheSets[cacheSetNumber][i].ApplyStateChange(&cacheset.CacheStateChange{Address: lru, NewState: cacheset.STATE_INVALID}) //TODO writeback upon evict if not in other caches (only if in M)

				if err != nil {
					panic(err)
				}
			}
		}
	}
}

func Writer(msgChan chan *[]string) {
	for true {
		msg := <-msgChan
		err := outWriter.Write(*msg)
		if err != nil {
			fmt.Println("Error writing:", err)
		}
	}
}
