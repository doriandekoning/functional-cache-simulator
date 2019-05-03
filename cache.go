package main



type cache interface {
	Set(key uint64)
	Get(key uint64)
}
