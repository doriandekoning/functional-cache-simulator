package reader

import (
	"bufio"
	"fmt"
	"io"
	"os"

	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
	"github.com/pkg/errors"

	"github.com/gogo/protobuf/proto"
)

const fileHeader = "gem5"

var addressMap map[uint64]uint64
var freePhysAddr = uint64(0)

const pageSize = uint64(4096)

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
	addressMap = make(map[uint64]uint64)

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
		return nil, errors.Wrap(err, "reading bytes")
	}
	if size != uint64(n) {
		return nil, fmt.Errorf("Not enough bytes read %d, %d", size, n)
	}
	return &bytes, nil
}

func (b *BufferedPBReader) ReadPacket() (*messages.Packet, error) {
	bytes, err := b.ReadBytes()
	if err == io.EOF {
		return nil, nil
	} else if err != nil {
		//TODO fix last packet cannot be read currently refactor reader to have `HasNext() bool` function
		return nil, errors.Wrap(err, "Reading bytes for packet")
	}
	packet := &messages.Packet{}
	if err := proto.Unmarshal(*bytes, packet); err != nil {
		return nil, errors.Wrap(err, "Unmarshalling packet")
	}
	retPacket := b.nextPacket
	b.nextPacket = packet

	addr := toPhysicalAddress(packet.GetAddr())
	retPacket.Addr = &addr
	return retPacket, nil
}

func toPhysicalAddress(address uint64) uint64 {
	page := (address >> 12)
	physAddr, found := addressMap[page]
	if found {
		return physAddr + (address % pageSize)
	}
	physAddr = freePhysAddr + (address % pageSize)
	freePhysAddr += pageSize
	addressMap[page] = physAddr
	return physAddr
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
