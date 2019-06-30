package reader

import "github.com/doriandekoning/functional-cache-simulator/pkg/messages"

type PBReader interface {
	ReadPacket() (*messages.Packet, error)
	GetHeader() *messages.PacketHeader
	NextTick() uint64
}

func ReadMultiple(r PBReader, n int) ([]*messages.Packet, error) {
	result := []*messages.Packet{}
	for i := 0; i < n; i++ {
		nextPacket, err := r.ReadPacket()
		if err != nil {
			return nil, err
		}
		if nextPacket == nil {
			return result, nil
		}
		result = append(result, nextPacket)
	}
	return result, nil
}
