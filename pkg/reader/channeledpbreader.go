package reader

import (
	"io"

	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

type ChanneledPBReader struct {
	reader  PBReader
	channel chan *messages.Packet
}

func NewChanneledPBReader(inputReader PBReader) (*ChanneledPBReader, error) {
	packetChannel := make(chan *messages.Packet, 1000) //TODO make channel size configurable
	channeledReader := &ChanneledPBReader{channel: packetChannel}

	channeledReader.reader = inputReader
	go channeledReader.PushPackets()
	return channeledReader, nil
}

func (c *ChanneledPBReader) PushPackets() {

	for true {
		nextPacket, err := c.reader.ReadPacket()
		if err == io.EOF {
			break
		} else if err != nil {
			panic(err)
		}
		if nextPacket == nil {
			break
		}
		c.channel <- nextPacket
	}
}

func (c *ChanneledPBReader) GetChannel() chan *messages.Packet {
	return c.channel
}
