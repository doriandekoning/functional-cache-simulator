package main

import (
	"testing"

	"github.com/stretchr/testify/suite"
)

type LRUCacheSuite struct {
	suite.Suite
	c *LRUCache
	m *Memory
}

func (suite *LRUCacheSuite) SetupTest() {
	suite.m = NewMemory()
	suite.c = NewLRUCache(5)
	suite.c.Memory = suite.m
}

func (s *LRUCacheSuite) TestCacheEmptyGetMiss() {
	s.False(s.c.Get(0))
}

func (s *LRUCacheSuite) TestCacheHit() {
	s.c.Set(0)
	s.True(s.c.Get(0))
}

func (s *LRUCacheSuite) TestCacheEvoke() {
	for i := uint64(0); i < 5; i++ {
		s.c.Set(i)
	}
	s.True(s.c.Get(0))  // LRU: [0, 4, 3, 2, 1]
	s.c.Set(5)          // LRU: [5, 0, 4, 3, 2]
	s.False(s.c.Get(1)) // LRU: [5, 0, 4, 3, 2]
	s.True(s.c.Get(0))  //LRU: [0, 5, 4, 3, 2]
}

func TestLRUCacheSuite(t *testing.T) {
	suite.Run(t, new(LRUCacheSuite))
}
