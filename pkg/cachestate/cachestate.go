package cachestate

import (
	"container/list"
	"fmt"
)

//State represents the state of a single lru MSI cache
type State struct {
	lruList    *list.List
	addressMap map[uint64]*AddressMapEntry
}

const (
	STATE_INVALID  = 0 //Invalid entries are not stored in the cache
	STATE_SHARED   = 1
	STATE_MODIFIED = 2
)

type CacheStateChange struct {
	evict *uint64
	put   uint64
}



type AddressMapEntry struct {
	listItem *list.Element
	state    int
}


func New() *State {
	return &State{
		lruList:    list.New(),
		addressMap: make(map[uint64]*AddressMapEntry),
	}
}

func (s *State) ApplyStateChange(stateChange *CacheStateChange) error {

	if stateChange.evict != nil {
		evictEntry, found := s.addressMap[*stateChange.evict]
		if !found {
			return fmt.Errorf("Item to evict not found in cache")
		}
		s.lruList.Remove(evictEntry.listItem)
		delete(s.addressMap, *stateChange.evict)
	}

	//Above it is checked if statechange.put == nil, therefore a check is not needed here
	_, found := s.addressMap[stateChange.put]
	if found {
		return fmt.Errorf("Trying to insert entry that is already in the cache")
	}

	//Put new entry in the list and the map
	s.putAddress(stateChange.put)
	return nil
}

func (s *State) putAddress(address uint64) {
	 listItem := s.lruList.PushFront(address)
	 s.addressMap[address] = &AddressMapEntry{listItem: listItem, state: STATE_SHARED}
}

func (s *State) Contains(address uint64) bool {
	_, contains := s.addressMap[address]
	return contains
}
