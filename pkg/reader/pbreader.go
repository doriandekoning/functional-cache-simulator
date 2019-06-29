package reader

import "github.com/doriandekoning/functional-cache-simulator/pkg/messages"

type PBReader interface {
	ReadPacket() (*Packet, error)
	GetHeader() *messages.PacketHeader
	NextTick() uint64
}

type Packet struct {
	*messages.Packet
	CpuID uint64
}

func ReadMultiple(r PBReader, n int) ([]*Packet, error) {
	result := []*Packet{}
	for i := 0; i < n; i++ {
		nextPacket, err := r.ReadPacket()
		if err != nil {
			return nil, err
		}
		if nextPacket == nil {
			return result, nil
		}
		nextPacket.CpuID = 0
		result = append(result, nextPacket)
	}
	return result, nil
}
