package cachestate

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type CacheStateTestSuite struct {
	suite.Suite
	state *State
}

func (suite *CacheStateTestSuite) SetupTest() {
	suite.state = New()
}

func (suite *CacheStateTestSuite) TestPutAddress() {
	suite.False(suite.state.Contains(1))
	suite.state.putAddress(1)
	suite.True(suite.state.Contains(1))
}

func (suite *CacheStateTestSuite) TestApplyStateDoubleInsertShouldError() {
	suite.state.putAddress(1)
	suite.Error(suite.state.ApplyStateChange(&CacheStateChange{put: 1}))
}

func (suite *CacheStateTestSuite) TestPutAddresPutsInfront() {
	suite.state.putAddress(1)
	suite.state.putAddress(2)
	suite.state.putAddress(3)
	suite.Equal(uint64(3), suite.state.lruList.Front().Value)
}

func (suite *CacheStateTestSuite) TestApplyStateChangeNoEvict() {
	suite.False(suite.state.Contains(6))
	suite.NoError(suite.state.ApplyStateChange(&CacheStateChange{put: 6}))
	suite.True(suite.state.Contains(6))
}

func (suite *CacheStateTestSuite) TestApplyStateChangeWithEvict() {
	suite.state.putAddress(9)
	evict := uint64(9)
	suite.NoError(suite.state.ApplyStateChange(&CacheStateChange{put: 2, evict: &evict}))
	suite.False(suite.state.Contains(9))
	suite.True(suite.state.Contains(2))
	suite.Equal(uint64(2), suite.state.lruList.Front().Value)
}

func (suite *CacheStateTestSuite) TestApplyStateChangeEvictNotInCacheError() {
	evict := uint64(8)
	suite.Error(suite.state.ApplyStateChange(&CacheStateChange{put: 2, evict: &evict}))
}

func TestRunCacheStateTestSuite(t *testing.T) {
	suite.Run(t, new(CacheStateTestSuite))
}
