package cachestate

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type CacheStateTestSuite struct {
	suite.Suite
	state *State
}

type TestCacheStateChange struct {
	changeType ChangeType
	put        uint64
}

func (t *TestCacheStateChange) GetChangeType() ChangeType {
	return t.changeType
}

func (t *TestCacheStateChange) GetPut() uint64 {
	return t.put
}

func (suite *CacheStateTestSuite) SetupTest() {
	suite.state = New(2)
}

func (suite *CacheStateTestSuite) TestPutAddress() {
	suite.False(suite.state.Contains(1))
	suite.state.putAddress(1)
	suite.True(suite.state.Contains(1))
}

func (suite *CacheStateTestSuite) TestApplyStateDoubleInsertShouldError() {
	suite.state.putAddress(1)
	suite.Error(suite.state.ApplyStateChange(&TestCacheStateChange{put: 1}))
}

func (suite *CacheStateTestSuite) TestPutAddresPutsInfront() {
	suite.state.putAddress(1)
	suite.state.putAddress(2)
	suite.state.putAddress(3)
	suite.Equal(uint64(3), suite.state.lruList.Front().Value)
}

func (suite *CacheStateTestSuite) TestApplyStateChangeNoEvict() {
	suite.False(suite.state.Contains(6))
	suite.NoError(suite.state.ApplyStateChange(&TestCacheStateChange{put: 6}))
	suite.True(suite.state.Contains(6))
}

func (suite *CacheStateTestSuite) TestApplyStateChangeWithEvict() {
	suite.state.putAddress(9)
	suite.NoError(suite.state.ApplyStateChange(&TestCacheStateChange{put: 2, changeType: ChangeTypeEvict}))
	suite.False(suite.state.Contains(9))
	suite.True(suite.state.Contains(2))
	suite.Equal(uint64(2), suite.state.lruList.Front().Value)
}

func (suite *CacheStateTestSuite) TestApplyStateChangeEvictNotInCacheError() {
	suite.Error(suite.state.ApplyStateChange(&TestCacheStateChange{put: 2, changeType: ChangeTypeEvict}))
}

func (suite *CacheStateTestSuite) TestApplyStateChangeEvictAndEntrySameItem() {
	suite.state.putAddress(8)
	suite.NoError(suite.state.ApplyStateChange(&TestCacheStateChange{put: 8, changeType: ChangeTypeEvict}))
}

func TestRunCacheStateTestSuite(t *testing.T) {
	suite.Run(t, new(CacheStateTestSuite))
}

//TODO test for cache size
