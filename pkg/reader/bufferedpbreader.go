package reader

import (
	"bufio"
	"fmt"
	"io"
	"os"

	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"

	"github.com/gogo/protobuf/proto"
)

const fileHeader = "gem5"

type BufferedPBReader struct {
	reader     bufio.Reader
	nextPacket *messages.Packet
	header     *messages.PacketHeader
}

func NewBufferedPBReader(path string) (*BufferedPBReader, error) {
	in, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	//TODO NewReaderSize
	reader := &BufferedPBReader{reader: *bufio.NewReader(in)}
	err = reader.checkHeader()
	if err != nil {
		return nil, err
	}
	reader.header, err = reader.readHeader()
	if err != nil {
		return nil, err
	}
	bytes, err := reader.ReadBytes()
	if err != nil {
		return nil, err
	}
	packet := &messages.Packet{}
	if err := proto.Unmarshal(*bytes, packet); err != nil {
		return nil, err
	}
	reader.nextPacket = packet

	return reader, nil
}

func (b *BufferedPBReader) checkHeader() error {
	headerLength := len([]byte(fileHeader))
	bytes := make([]byte, headerLength)
	n, err := io.ReadFull(&b.reader, bytes)
	if err != nil || n != headerLength {
		return fmt.Errorf("Error reading header")
	}
	header := string(bytes)
	if header != fileHeader {
		return fmt.Errorf("Header not correct")
	}
	return nil
}

func (b *BufferedPBReader) ReadVarInt() (uint64, error) {
	bytes, err := b.reader.Peek(8)
	if err != nil {
		return 0, err
	}
	ret, n := proto.DecodeVarint(bytes)
	if n == 0 {
		return 0, fmt.Errorf("Error decoding varint")
	}
	discarded, err := b.reader.Discard(n)
	if n != discarded {
		return 0, fmt.Errorf("Discarded not equal to read")
	}
	return ret, nil
}

func (b *BufferedPBReader) ReadBytes() (*[]byte, error) {
	size, err := b.ReadVarInt()
	if err != nil {
		return nil, err
	}
	bytes := make([]byte, size)
	n, err := io.ReadFull(&b.reader, bytes)
	//TODO handle EOF
	if err != nil {
		return nil, err
	}
	if size != uint64(n) {
		return nil, fmt.Errorf("Not enough bytes read %d, %d", size, n)
	}
	return &bytes, nil
}

func (b *BufferedPBReader) ReadPacket() (*Packet, error) {
	bytes, err := b.ReadBytes()
	if err == io.EOF {
		return nil, nil
	} else if err != nil {
		//TODO fix last packet cannot be read currently refactor reader to have `HasNext() bool` function
		return nil, err
	}
	packet := &messages.Packet{}
	if err := proto.Unmarshal(*bytes, packet); err != nil {

		return nil, err
	}
	retPacket := b.nextPacket
	b.nextPacket = packet

	return &Packet{Packet: retPacket}, nil
}

func (b *BufferedPBReader) GetHeader() *messages.PacketHeader {
	return b.header
}

func (b *BufferedPBReader) readHeader() (*messages.PacketHeader, error) {
	bytes, err := b.ReadBytes()
	if err != nil {
		return nil, err
	}
	packet := &messages.PacketHeader{}
	err = proto.Unmarshal(*bytes, packet)
	if err != nil {
		return nil, err
	}
	return packet, nil

}

func (b *BufferedPBReader) NextTick() uint64 {
	if b.nextPacket != nil {
		return b.nextPacket.GetTick()
	}
	return 0
}
