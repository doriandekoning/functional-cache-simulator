package main

import (
	"container/list"
)

type LRUCache struct {
	items *list.List
	values map[uint64]*list.Element
	Size int
	Hits uint64
	Misses uint64
	Writes uint64
	Evictions uint64
}

func NewLRUCache(size int) *LRUCache {
	return &LRUCache{
		items: list.New(),
		values: make(map[uint64]*list.Element),
		Size: size,
	}
}

func (c *LRUCache) Get(key uint64) bool {
	var hit bool
	if line, hit := c.values[key]; hit {
		c.items.MoveToFront(line)
		c.Hits++
	} else {
		c.Misses++
	}
	return hit
}


func (c *LRUCache) Set(key uint64) {
	if line, contains := c.values[key]; contains {
		c.items.MoveToFront(line)
		c.Writes++
	} else {
		c.values[key] = c.items.PushFront(key)
		//Check size
		if len(c.values) > c.Size {
			lru := c.items.Back()
			delete(c.values, key)
			c.items.Remove(lru)
			c.Evictions++
		}
	}
}
