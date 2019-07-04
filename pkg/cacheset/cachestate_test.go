package cacheset

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type CacheStateTestSuite struct {
	suite.Suite
	state *State
}

func TestRunCacheStateTestSuite(t *testing.T) {
	suite.Run(t, new(CacheStateTestSuite))
}

type TestCacheStateChange struct {
	newState CacheLineState
	address  uint64
}

func (t *TestCacheStateChange) GetAddress() uint64 {
	return t.address
}

func (t *TestCacheStateChange) GetNewState() CacheLineState {
	return t.newState
}

func (suite *CacheStateTestSuite) SetupTest() {
	suite.state = New(2)
}

func (suite *CacheStateTestSuite) TestPutAddress() {
	suite.NotEqual(STATE_INVALID, suite.state.GetState(1))
	suite.state.putAddress(1, STATE_SHARED)
	suite.Equal(STATE_SHARED, suite.state.GetState(1))
}

func (suite *CacheStateTestSuite) TestApplyStateDoubleInsertShouldError() {
	suite.state.putAddress(1, STATE_SHARED)
	suite.Error(suite.state.ApplyStateChange(&TestCacheStateChange{address: 1}))
}

func (suite *CacheStateTestSuite) TestMSIStateChange() {
	var tests = []struct {
		name       string
		curState   CacheLineState
		write      bool
		newState   CacheLineState
		busRequest BusRequest
	}{
		{
			name:       "InvalidPrRd",
			curState:   STATE_INVALID,
			write:      false,
			newState:   STATE_SHARED,
			busRequest: BUS_READ,
		},
		{
			name:       "InvalidPrWr",
			curState:   STATE_INVALID,
			write:      true,
			newState:   STATE_MODIFIED,
			busRequest: BUS_READX,
		},
		{
			name:       "SharedPrRd",
			curState:   STATE_SHARED,
			write:      false,
			newState:   STATE_SHARED,
			busRequest: BUS_NOTHING,
		},
		{
			name:       "SharedPrWr",
			curState:   STATE_SHARED,
			write:      true,
			newState:   STATE_MODIFIED,
			busRequest: BUS_UPGR,
		},
		{
			name:       "ModifiedPrRd",
			curState:   STATE_MODIFIED,
			write:      false,
			newState:   STATE_MODIFIED,
			busRequest: BUS_NOTHING,
		},
		{
			name:       "ModifiedPrWr",
			curState:   STATE_MODIFIED,
			write:      true,
			newState:   STATE_MODIFIED,
			busRequest: BUS_NOTHING,
		},
	}
	for _, test := range tests {
		suite.Run(test.name, func() {
			addressState, busRequest := GetMSIStateChange(test.curState, test.write)
			suite.Equal(test.newState, addressState)
			suite.Equal(test.busRequest, busRequest)
		})
	}
}

func (suite *CacheStateTestSuite) TestMSIStateBusRequestChange() {
	var tests = []struct {
		name       string
		curState   CacheLineState
		busRequest BusRequest
		newState   CacheLineState
		flush      bool
	}{
		{
			name:       "InvalidBusRd",
			curState:   STATE_INVALID,
			busRequest: BUS_READ,
			newState:   STATE_INVALID,
			flush:      false,
		},
		{
			name:       "InvalidBusRdX",
			curState:   STATE_INVALID,
			busRequest: BUS_READX,
			newState:   STATE_INVALID,
			flush:      false,
		},
		{
			name:       "InvalidBusUpgr",
			curState:   STATE_INVALID,
			busRequest: BUS_UPGR,
			newState:   STATE_INVALID,
			flush:      false,
		},
		{
			name:       "SharedBusRd",
			curState:   STATE_SHARED,
			busRequest: BUS_READ,
			newState:   STATE_SHARED,
			flush:      false,
		},
		{
			name:       "SharedBusRdX",
			curState:   STATE_SHARED,
			busRequest: BUS_READX,
			newState:   STATE_INVALID,
			flush:      false,
		},
		{
			name:       "SharedBusUpgr",
			curState:   STATE_SHARED,
			busRequest: BUS_UPGR,
			newState:   STATE_INVALID,
			flush:      false,
		},
		{
			name:       "ModifiedBusRd",
			curState:   STATE_MODIFIED,
			busRequest: BUS_READ,
			newState:   STATE_SHARED,
			flush:      true,
		},
		{
			name:       "ModifiedBusRdX",
			curState:   STATE_MODIFIED,
			busRequest: BUS_READX,
			newState:   STATE_INVALID,
			flush:      true,
		},
	}
	for _, test := range tests {
		suite.Run(test.name, func() {
			addressState, flush := GetMSIStateChangeByBusRequest(test.curState, test.busRequest)
			suite.Equal(test.newState, addressState)
			suite.Equal(test.flush, flush)
		})
	}
}
