package main

import (
	"flag"
	"fmt"
	"io/ioutil"
	"log"

	pb "github.com/doriandekoning/functional-cache-simulator/messages"


	"github.com/gogo/protobuf/proto"
)

const fileHeader = "gem5"
const cacheLineSize = 64 // Cache line size in bytes
const L1CacheSize = 64 * 1024
var debuggingEnabled = false

func main() {
	inputFile := flag.String("input", "", "Input trace file to analyse")
	flag.BoolVar(&debuggingEnabled, "debug", false, "If set to true additional debugging info will be logged")
	flag.Parse()

	if inputFile == nil {
	    panic("No input file provided")
	}

	in, err := ioutil.ReadFile(*inputFile)
	if err != nil {
		log.Fatalln("Error reading file:", err)
	}
	headerLength := len([]byte(fileHeader))
	header := string(in[:4])
	if header != fileHeader {
		panic("Input not recognized")
	}
	curOffset := headerLength
	traceHeader := &pb.PacketHeader{}
	nextMessagesize, n := proto.DecodeVarint(in[curOffset:])
	curOffset += n
	if err := proto.Unmarshal(in[curOffset:(curOffset+int(nextMessagesize))], traceHeader); err != nil {
		panic(err)
	}
	curOffset += int(nextMessagesize)
	cache := NewLRUCache(L1CacheSize / cacheLineSize)
	for true {
		nextMessagesize, n := proto.DecodeVarint(in[curOffset:])
		curOffset += n
		if curOffset+int(nextMessagesize) >= len(in) {
			break
		}
		trace := &pb.Packet{}
		if err := proto.Unmarshal(in[curOffset:(curOffset+int(nextMessagesize))], trace); err != nil {
			panic(err)
		}
		curOffset += int(nextMessagesize)
		cacheLine := trace.GetAddr() - (trace.GetAddr() % cacheLineSize)
		if trace.GetCmd() == 1 {
			//TODO handle multi line reads
			cache.Get(cacheLine)
		} else {
			cache.Set(cacheLine)
		}
	}
	fmt.Println("-----------------------\nTrace statistics:")
	fmt.Println("Writes:", cache.Writes)
	fmt.Println("Misses:", cache.Misses)
	fmt.Println("Evictions:", cache.Evictions)
	fmt.Println("Hits:", cache.Hits)
	fmt.Println("-----------------------")
}

