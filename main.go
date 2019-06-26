package main

import (
	"encoding/csv"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strconv"
	"strings"
)

const fileHeader = "gem5"
const cacheLineSize = 64 // Cache line size in bytes
const cacheSize = 64     // Cache size in lines

var debuggingEnabled = false
var batch = false
var batchSize = 1
var outWriter *csv.Writer

type Stats struct {
	MemoryReads      uint64
	MemoryWrites     uint64
	LocalCacheMisses uint64
	CacheWrites      uint64
	CacheMisses      uint64
	CacheHits        uint64
	CacheEvictions   uint64
}

func main() {
	inputFiles := flag.String("inputs", "", "Input trace file to analyse")
	outputFileLoc := flag.String("output", "", "Output")
	flag.BoolVar(&debuggingEnabled, "debug", false, "If set to true additional debugging info will be logged")
	flag.BoolVar(&batch, "batch", false, "If set to true batch processing will be used (less accurate but probably faster)")
	flag.IntVar(&batchSize, "batchsize", 1, "Sets the batchsize, a larger value is faster but less accurate, a batch size of 1 results in the exact result")
	flag.Parse()

	if inputFiles == nil {
		panic("No input file provided")
	}

	inputFilesSplit := strings.Split(*inputFiles, ",")
	inputReaders := make(map[int]*BufferedPBReader)
	for i, inputF := range inputFilesSplit {
		in, err := NewReader(inputF)
		if err != nil {
			panic(err)
		}

		_, err = in.ReadHeader()
		if err != nil {
			panic(err)
		}
		inputReaders[i] = in
	}

	var outFile io.Writer
	if outputFileLoc == nil {
		fmt.Println("WARNING: Writing all output to discard writer because no output file was specified")
		outFile = ioutil.Discard
	} else {
		var err error
		outFile, err = os.Create(*outputFileLoc)
		if err != nil {
			panic(fmt.Sprintf("Error while creating output file", err))
		}
	}

	outWriter = csv.NewWriter(outFile)
	var stats *Stats
	if !batch {
		stats = simulateSequential(inputReaders[0], outWriter)
	} else {
		stats = simulateBatch(inputReaders, outWriter)
	}

	fmt.Println("-----------------------\nTrace statistics:")
	fmt.Println("Cache Writes:", stats.CacheWrites)
	fmt.Println("Cache Misses:", stats.CacheMisses)
	fmt.Println("Cache Evictions:", stats.CacheEvictions)
	fmt.Println("Cache Hits:", stats.CacheHits)
	fmt.Println("Memory Writes:", stats.MemoryWrites)
	fmt.Println("Memory Reads:", stats.MemoryReads)
	fmt.Println("-----------------------")
}

func simulateSequential(input *BufferedPBReader, outFile *csv.Writer) *Stats {

	memory := NewMemory()
	cache := NewLRUCache(cacheSize)
	cache.Memory = memory
	someCounter := 0
	for true {
		packet, err := input.ReadPacket()
		if err == io.EOF {
			fmt.Println("EOF reached")
			break
		} else if err != nil {
			panic(err)
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		if packet.GetCmd() == 1 {

			var hm = "h"
			hit := cache.Get(cacheLine)
			if !hit {
				someCounter++

				hm = "m"
				cache.Set(cacheLine)

			}
			outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), hm})
		} else {
			cache.Set(cacheLine)
			//Assume write through cache, where each write is written back to memory (in the order the cache received them)
			outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), "h"})
		}

	}
	fmt.Println("someCounter:", someCounter)
	outWriter.Flush()
	return &Stats{
		MemoryReads:    memory.Reads,
		MemoryWrites:   memory.Writes,
		CacheWrites:    cache.Writes,
		CacheMisses:    cache.Misses,
		CacheHits:      cache.Hits,
		CacheEvictions: cache.Evictions,
	}
}
