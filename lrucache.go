package main

import (
	"container/list"
)

type CacheEntry struct {
	address uint64
	dirty   bool //TODO maybe this can be encoded in the msb of the address
}

type LRUCache struct {
	items     *list.List
	values    map[uint64]*list.Element
	Size      int
	Hits      uint64
	Misses    uint64
	Writes    uint64
	Evictions uint64
	Memory    *Memory
}

func NewLRUCache(size int) *LRUCache {
	return &LRUCache{
		items:  list.New(),
		values: make(map[uint64]*list.Element),
		Size:   size,
	}
}

func (c *LRUCache) Get(addr uint64) bool {
	var hit bool
	var line *list.Element
	if line, hit = c.values[addr]; hit {
		c.items.MoveToFront(line)
		c.Hits++
	} else {
		c.addEntry(addr, false)
		c.Memory.Read(addr)
		c.Misses++
	}
	return hit
}

func (c *LRUCache) Set(addr uint64) {
	if line, contains := c.values[addr]; contains {
		c.items.MoveToFront(line)
		line.Value.(*CacheEntry).dirty = true
		c.Memory.Write(addr) // Write through cache
		c.Writes++
	} else {
		c.addEntry(addr, true)
	}
}

func (c *LRUCache) addEntry(addr uint64, dirty bool) {
	entry := &CacheEntry{address: addr, dirty: dirty}
	c.values[addr] = c.items.PushFront(entry)
	//Check size
	if len(c.values) > c.Size {
		if c.values[addr].Value.(*CacheEntry).dirty {
			c.Memory.Write(addr)
		}
		c.evict()
	}
}

func (c *LRUCache) evict() {
	lru := c.items.Back()
	delete(c.values, lru.Value.(*CacheEntry).address)
	c.items.Remove(lru)
	c.Evictions++
}
