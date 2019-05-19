package main

import (
	"os"
	"strconv"
	"encoding/csv"
)

type Memory struct {
 	outWriter *csv.Writer
	Writes uint64
	Reads  uint64
}

func NewMemory(outputLoc string) *Memory {
	file, err := os.Create(outputLoc)
	if err != nil {
	    panic(err)
	}
	mem := Memory{}
	mem.outWriter = csv.NewWriter(file)
	return &mem
}

func (m *Memory) Read(address uint64) {
	m.Reads++
	m.outWriter.Write([]string{strconv.FormatUint(address, 10), "w"})
}

func (m *Memory) Write(address uint64) {
	m.Writes++
	m.outWriter.Write([]string{strconv.FormatUint(address, 10), "r"})
}
