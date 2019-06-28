package reader

import (
	"container/list"
	"fmt"
	"io"

	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

type MemoryPbReader struct {
	header  *messages.PacketHeader
	packets *list.List
}

func NewMemoryPBReader(path string) (*MemoryPbReader, error) {
	packets, header, err := readCompleteFile(path)
	if err != nil {
		return nil, err
	}
	fmt.Println("Finished reading complete file, read:", packets.Len(), " packets")
	return &MemoryPbReader{header: header, packets: packets}, nil
}

func (r *MemoryPbReader) ReadPacket() (*messages.Packet, error) {
	if front := r.packets.Front(); front != nil {
		return r.packets.Remove(front).(*messages.Packet), nil
	}
	return nil, nil
}

func (r *MemoryPbReader) GetHeader() *messages.PacketHeader {
	return r.header
}

func (r *MemoryPbReader) NextTick() uint64 {
	front := r.packets.Front()
	if front == nil || front.Value == nil {
		return 0
	}
	return *front.Value.(*messages.Packet).Tick
}

func readCompleteFile(path string) (*list.List, *messages.PacketHeader, error) {
	packets := list.New()
	bufReader, err := NewBufferedPBReader(path)
	if err != nil {
		return nil, nil, err
	}
	i := 0
	for true {
		nextPacket, err := bufReader.ReadPacket()
		if err == io.EOF {
			break
		} else if err != nil {
			return nil, nil, err
		}
		if nextPacket == nil {
			break
		}
		packets.PushBack(nextPacket)
		i++
	}
	return packets, bufReader.GetHeader(), nil
}
