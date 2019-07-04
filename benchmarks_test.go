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

type testStruct struct {
	iets   int
	anders int
}

func BenchmarkInitWithSize(b *testing.B) {
	arr := make([]*testStruct, b.N)
	for i := 0; i < b.N; i++ {
		arr[i] = &testStruct{iets: (i - 1), anders: (i + 7)}
	}
}

func BenchmarkInitAppend(b *testing.B) {
	arr := []*testStruct{}
	for i := 0; i < b.N; i++ {
		arr = append(arr, &testStruct{iets: (i - 1), anders: (i + 7)})
	}
}
