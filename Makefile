
# Makefile for compression routines
# 2015-05-24: started by David Cary


default: time_test

time_test: small_compression
	time --verbose ./small_compression

CFLAGS += -Wall

.PHONY: clean
clean:
	rm small_compression


