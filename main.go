package main

import (
	"encoding/csv"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"strconv"
)

const fileHeader = "gem5"
const cacheLineSize = 64 // Cache line size in bytes
const L1CacheSize = 64 * 1024

var debuggingEnabled = false
var outWriter *csv.Writer

func main() {
	inputFile := flag.String("input", "", "Input trace file to analyse")
	outputFileLoc := flag.String("output", "", "Output")
	flag.BoolVar(&debuggingEnabled, "debug", false, "If set to true additional debugging info will be logged")
	flag.Parse()

	if inputFile == nil {
		panic("No input file provided")
	}

	in, err := NewReader(*inputFile)
	if err != nil {
		panic(err)
	}
	var outFile io.Writer
	if outputFileLoc == nil {
		fmt.Println("WARNING: Writing all output to discard writer because no output file was specified")
		outFile = ioutil.Discard
	} else {
		outFile, err = os.Create(*outputFileLoc)
		if err != nil {
			panic(fmt.Sprintf("Error while creating output file", err))
		}
	}
	outWriter = csv.NewWriter(outFile)

	header, err := in.ReadHeader()
	if err != nil {
		panic(err)
	}
	fmt.Printf("%+v\n", header)

	memory := NewMemory()
	cache := NewLRUCache(L1CacheSize / cacheLineSize)
	cache.Memory = memory
	for true {
		packet, err := in.ReadPacket()
		if err == io.EOF {
			fmt.Println("EOF reached")
			break
		} else if err != nil {
			panic(err)
		}
		cacheLine := packet.GetAddr() - (packet.GetAddr() % cacheLineSize)
		var op = "nop"
		if packet.GetCmd() == 1 {
			//TODO handle multi line reads
			hit := cache.Get(cacheLine)
			if !hit {
				op = "r"
			}
		} else {
			cache.Set(cacheLine)
			//Assume write through cache, where each write is written back to memory (in the order the cache received them)
			op = "w"
		}
		outWriter.Write([]string{strconv.FormatUint(packet.GetAddr(), 10), "r", op})
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
