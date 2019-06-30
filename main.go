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
	"time"

	"github.com/doriandekoning/functional-cache-simulator/pkg/reader"
)

const cacheLineSize = 64 // Cache line size in bytes
const cacheSize = 8196   // Cache size in lines

var debuggingEnabled = false
var bufferCompleteFile = false
var simulator = ""
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
	numThreads := flag.Int("threads", 1, "Specify the amount of threads to use for concurrents simulation")
	flag.BoolVar(&debuggingEnabled, "debug", false, "If set to true additional debugging info will be logged")
	flag.StringVar(&simulator, "simulator", "sequential", "Selects the simulator to use, options are: sequential(s), batch(b) or concurrent(c)")
	flag.BoolVar(&bufferCompleteFile, "buffer-complete-file", false, "If set to true the complete file is read into memory before simulating")
	flag.IntVar(&batchSize, "batchsize", 1, "Sets the batchsize, a larger value is faster but less accurate, a batch size of 1 results in the exact result")
	flag.Parse()

	if inputFiles == nil {
		panic("No input file provided")
	}
	if bufferCompleteFile {
		fmt.Println("Reading the complete trace into memory before starting the simulation")
	}
	inputFilesSplit := strings.Split(*inputFiles, ",")
	inputReaders := make(map[int]reader.PBReader)
	for i, inputF := range inputFilesSplit {
		var in reader.PBReader
		var err error
		if !bufferCompleteFile {
			in, err = reader.NewBufferedPBReader(inputF)
			if err != nil {
				fmt.Println(inputF)
				panic(err)
			}

		} else {
			in, err = reader.NewMemoryPBReader(inputF)
		}
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
			panic(fmt.Sprintf("Error while creating output file, %v", err))
		}
	}

	outWriter = csv.NewWriter(outFile)
	var stats *Stats
	t0 := time.Now()
	fmt.Printf("Using %s simulator\n", simulator)
	if simulator == "sequential" || simulator == "s" {
		stats = simulateSequential(inputReaders[0], outWriter)
	} else if simulator == "batch" || simulator == "b" {
		// stats = simulateBatch(inputReaders, outWriter)
	} else if simulator == "concurrent" || simulator == "c" {
		stats = SimulateConcurrent(inputReaders, outWriter, *numThreads, batchSize)
	} else {
		fmt.Printf("Simulator type %s not known", simulator)
		return
	}
	fmt.Println("Time spend on simulation: ", time.Since(t0))

	fmt.Println("-----------------------\nTrace statistics:")
	fmt.Println("Cache Writes:", stats.CacheWrites)
	fmt.Println("Cache Misses:", stats.CacheMisses)
	fmt.Println("Cache Evictions:", stats.CacheEvictions)
	fmt.Println("Cache Hits:", stats.CacheHits)
	fmt.Println("Memory Writes:", stats.MemoryWrites)
	fmt.Println("Memory Reads:", stats.MemoryReads)
	fmt.Println("-----------------------")
}

func simulateSequential(input reader.PBReader, outFile *csv.Writer) *Stats {

	memory := NewMemory()
	cache := NewLRUCache(cacheSize)
	cache.Memory = memory
	for true {
		packet, err := input.ReadPacket()
		if packet == nil {
			fmt.Println("Simulated all packets")
			break
		} else if err != nil {
			panic(err)
		} else if packet == nil {
			break
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		if packet.GetCmd() == 1 {

			var hm = "h"
			hit := cache.Get(cacheLine)
			if !hit {
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
