package main


import (
	"testing"
	"github.com/stretch/testify/assert"
)

func TestCacheEmptyGetMiss(t *testing.T) {
	c := LRUCache()
	assert.False(t, c.Get(0))
}
