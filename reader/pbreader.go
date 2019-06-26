package reader

import pb "github.com/doriandekoning/functional-cache-simulator/messages"

type PBReader interface {
	ReadPacket() (*pb.Packet, error)
	GetHeader() *pb.PacketHeader
	NextTick() uint64
}
