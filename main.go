package main

import (
	"flag"
	"fmt"
	"io"
)

const fileHeader = "gem5"
const cacheLineSize = 64 // Cache line size in bytes
const L1CacheSize = 64 * 1024
var debuggingEnabled = false

func main() {
	inputFile := flag.String("input", "", "Input trace file to analyse")
	outputFile := flag.String("output", "", "Output")
	flag.BoolVar(&debuggingEnabled, "debug", false, "If set to true additional debugging info will be logged")
	flag.Parse()

	if inputFile == nil {
	    panic("No input file provided")
	}

	if outputFile == nil {
	    panic("No output file provided")
	}

	in, err := NewReader(*inputFile)
	if err != nil {
		panic(err)
	}

	header, err := in.ReadHeader()
	if err != nil {
	    panic(err)
	}
	fmt.Printf("%+v\n", header)

	memory := NewMemory(*outputFile)
	cache := NewLRUCache(L1CacheSize / cacheLineSize)
	cache.Memory = memory
	for true {
		packet, err := in.ReadPacket()
		if err == io.EOF{
		    fmt.Println("EOF reached")
		    break
		} else if err != nil {
		    panic(err)
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		if packet.GetCmd() == 1 {
			//TODO handle multi line reads
			cache.Get(cacheLine)
		} else {
			cache.Set(cacheLine)
		}
	}
	fmt.Println("-----------------------\nTrace statistics:")
	fmt.Println("L1 Writes:", cache.Writes)
	fmt.Println("L1 Misses:", cache.Misses)
	fmt.Println("L1 Evictions:", cache.Evictions)
	fmt.Println("L1 Hits:", cache.Hits)
	fmt.Println("Memory Writes:", memory.Writes)
	fmt.Println("Memory Reads:", memory.Reads)
	fmt.Println("-----------------------")
}

