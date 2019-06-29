package cachestate

import (
	"container/list"
)

//State represents the state of a single lru MSI cache
type State struct {
	lruList    *list.List
	addressMap map[uint64]*AddressMapEntry
	size       int
}

type ChangeType int

const (
	ChangeTypeEvict     = 1
	ChangeTypeMoveFront = 2
)

const (
	STATE_INVALID  = 0 //Invalid entries are not stored in the cache
	STATE_SHARED   = 1
	STATE_MODIFIED = 2
)

//TODO maybe change to include state
type CacheStateChange interface {
	//Just specify if we need to evict or move forward, the order of the execution of these should not matter
	GetChangeType() ChangeType
	GetPut() uint64
}

type AddressMapEntry struct {
	listItem *list.Element
	state    int
}

func New(size int) *State {
	return &State{
		lruList:    list.New(),
		addressMap: make(map[uint64]*AddressMapEntry),
		size:       size,
	}
}

func stateChangeIsEvict(s CacheStateChange) bool {
	return s.GetChangeType() == ChangeTypeEvict
}

func (s *State) ApplyStateChange(stateChange CacheStateChange) error {
	entry, found := s.addressMap[stateChange.GetPut()]

	if !found || stateChangeIsEvict(stateChange) {
		_, found := s.addressMap[stateChange.GetPut()]
		if !found {
			if s.lruList.Len() >= s.size {
				delete(s.addressMap, s.lruList.Back().Value.(uint64))
				s.lruList.Remove(s.lruList.Back())
			}
			//Put new entry in the list and the map
			s.putAddress(stateChange.GetPut())
		}

	} else if !stateChangeIsEvict(stateChange) {
		s.lruList.MoveBefore(entry.listItem, s.lruList.Front())
	}

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
