package main

type Memory struct {
	//	outWriter *csv.Writer
	Writes uint64
	Reads  uint64
}

func NewMemory() *Memory {
	mem := Memory{}
	return &mem
}

func (m *Memory) Read(address uint64) {
	m.Reads++
}

func (m *Memory) Write(address uint64) {
	m.Writes++
}
