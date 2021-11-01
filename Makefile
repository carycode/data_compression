
# Makefile for compression routines
# 2015-05-24: started by David Cary


default: time_test

time_test: small_time_test n_ary_time_test

n_ary_time_test: n_ary_huffman
	time --verbose --output=n_ary_time_test ./n_ary_huffman

small_time_test: small_compression
	time --verbose --output=small_time_test ./small_compression

CFLAGS += -Wall

.PHONY: clean
clean:
	rm small_compression
	rm n_ary_huffman


