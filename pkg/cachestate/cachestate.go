package cachestate

import (
	"container/list"
	"fmt"
)

//State represents the state of a single lru MSI cache
type State struct {
	lruList    *list.List
	addressMap map[uint64]*AddressMapEntry
	size       int
}

type CacheLineState int

const (
	STATE_INVALID  = 0 //Invalid entries are not stored in the cache
	STATE_SHARED   = 1
	STATE_MODIFIED = 2
)

type BusRequest int

const (
	BUS_NOTHING = 0
	BUS_READ    = 1
	BUS_READX   = 2
	BUS_UPGR    = 3
	FLUSH       = 4
)

type CacheStateChange interface {
	GetNewState() CacheLineState
	GetAddress() uint64
}

type AddressMapEntry struct {
	listItem *list.Element
	state    CacheLineState
}

func New(size int) *State {
	return &State{
		lruList:    list.New(), //TODO see if we can get rid of this list
		addressMap: make(map[uint64]*AddressMapEntry),
		size:       size,
	}
}

func (s *State) ApplyStateChange(stateChange CacheStateChange) error {
	if stateChange.GetNewState() == STATE_INVALID {
		s.removeAddress(stateChange.GetAddress())
	} else {
		entry, contains := s.addressMap[stateChange.GetAddress()]
		if contains {
			s.lruList.MoveToBack(entry.listItem)
			entry.state = stateChange.GetNewState()
		} else {
			s.putAddress(stateChange.GetAddress(), stateChange.GetNewState())
		}
	}

	return nil
}

func (s *State) removeAddress(address uint64) {
	//Should always be found otherwise panic
	entry, found := s.addressMap[address]
	if !found {
		fmt.Errorf("Trying to remove entry which is not in the map")
	}
	s.lruList.Remove(entry.listItem)
	delete(s.addressMap, address)
}

func (s *State) putAddress(address uint64, state CacheLineState) {
	listItem := s.lruList.PushFront(address)
	s.addressMap[address] = &AddressMapEntry{listItem: listItem, state: state}
}

func (s *State) GetState(address uint64) CacheLineState {
	item, contains := s.addressMap[address]
	if !contains {
		return STATE_INVALID
	}
	return item.state
}

func (s *State) Contains(address uint64) bool {
	_, contains := s.addressMap[address]
	return contains
}

func (s *State) Copy() State {
	newList := list.New()
	newMap := make(map[uint64]*AddressMapEntry)
	nextItem := s.lruList.Front()
	for nextItem != nil {
		newListItem := newList.PushBack(nextItem.Value)
		newMap[nextItem.Value.(uint64)] = &AddressMapEntry{
			listItem: newListItem,
			state:    s.GetState(nextItem.Value.(uint64)),
		}
		nextItem = nextItem.Next()
	}
	return State{lruList: newList, size: s.size, addressMap: newMap}

}

func GetMSIStateChange(currentState CacheLineState, write bool) (CacheLineState, BusRequest) {
	if write {
		if currentState == STATE_INVALID {
			//PrWrI: accessing processor to M and issues BusRdX
			return STATE_MODIFIED, BUS_READX
		} else if currentState == STATE_SHARED {
			//PrWrS: that processor to M others to I
			return STATE_MODIFIED, BUS_UPGR
		} else if currentState == STATE_MODIFIED {
			//PrWrM: accessing processor to M others stays same
			return STATE_MODIFIED, BUS_NOTHING
		} else {
			fmt.Println("This transition is not known")
			return -1, -1
		}
	} else {
		if currentState == STATE_INVALID {
			//PrRdI: accessing processor to S others to S
			return STATE_SHARED, BUS_READ
		} else if currentState == STATE_SHARED {
			return STATE_SHARED, BUS_NOTHING
		} else if currentState == STATE_MODIFIED {
			return STATE_MODIFIED, BUS_NOTHING
		} else {
			fmt.Println("This transition is not known")
			return -1, -1
		}
	}
	return -1, -1
}

func GetMSIStateChangeByBusRequest(currentState CacheLineState, busRequest BusRequest) (newState CacheLineState, flush bool) {
	if currentState == STATE_INVALID {
		return STATE_INVALID, false
	} else if currentState == STATE_SHARED {
		if busRequest == BUS_READ {
			return STATE_SHARED, false
		} else {
			return STATE_INVALID, false
		}
	} else if currentState == STATE_MODIFIED {
		if busRequest == BUS_READ {
			return STATE_SHARED, true
		} else {
			return STATE_INVALID, true
		}
	} else {
		fmt.Println("This transition is not known")
		return -1, true
	}
}
