package main

import (
	"container/list"
	"testing"

	"github.com/doriandekoning/functional-cache-simulator/pkg/messages"
)

func BenchmarkAppend(b *testing.B) {
	arr := []*messages.Packet{}
	for i := 0; i < b.N; i++ {
		arr = append(arr, &messages.Packet{})
	}
}

func BenchmarkList(b *testing.B) {
	l := list.New()
	for i := 0; i < b.N; i++ {
		l.PushBack(&messages.Packet{})
	}
}
