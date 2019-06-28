package reader

import "github.com/doriandekoning/functional-cache-simulator/pkg/messages"

type PBReader interface {
	ReadPacket() (*messages.Packet, error)
	GetHeader() *messages.PacketHeader
	NextTick() uint64
}
