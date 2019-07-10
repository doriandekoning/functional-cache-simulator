package cacheset

import (
	"container/list"
	"fmt"
)

type CacheEntry struct {
	address uint64
	state   CacheLineState
}

//State represents the state of a single lru MSI cache
type State struct {
	lruList *list.List
	size    int
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

type CacheStateChange struct {
	NewState  CacheLineState
	Address   uint64
	Timestamp uint64
}

func New(size int) *State {
	return &State{
		lruList: list.New(),
		size:    size,
	}
}

func (s *State) ApplyStateChange(stateChange *CacheStateChange) error {
	if stateChange.NewState == STATE_INVALID {
		s.removeAddress(stateChange.Address)
	} else {
		entry := s.GetListItem(stateChange.Address)
		if entry != nil {
			s.lruList.MoveToBack(entry)
			entry.Value.(*CacheEntry).state = stateChange.NewState
		} else {
			s.putAddress(stateChange.Address, stateChange.NewState)
		}
	}

	return nil
}

//Retrieve address of LRU
func (s *State) GetLRU() uint64 {
	return s.lruList.Back().Value.(CacheEntry).address
}

func (s *State) GetInUse() int {
	return s.lruList.Len()
}

func (s *State) removeAddress(address uint64) {
	//Should always be found otherwise panic
	li := s.GetListItem(address)
	if li == nil {
		panic("Trying to remove entry which is not in the cache")
	}
	s.lruList.Remove(li)
}

func (s *State) putAddress(address uint64, state CacheLineState) {
	s.lruList.PushFront(&CacheEntry{address: address, state: state})
}

func (s *State) GetState(address uint64) CacheLineState {
	entry := s.GetListItem(address)
	if entry == nil {
		return STATE_INVALID
	}
	return entry.Value.(*CacheEntry).state
}

func (s *State) GetListItem(address uint64) *list.Element {
	nextItem := s.lruList.Front()
	for nextItem != nil {
		if nextItem.Value.(*CacheEntry).address == address {
			return nextItem
		}
		nextItem = nextItem.Next()
	}
	return nil
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
