
# Makefile for compression routines
# 2015-05-24: started by David Cary


# C compiler options recommended by
# https://software.codidact.com/posts/282565
CFLAGS := $(CFLAGS) -std=c17 -pedantic-errors -Wall -Wextra -Werror -Wunused-result

default: time_test

time_test: small_time_test n_ary_time_test

n_ary_time_test: n_ary_huffman
	time --verbose --output=n_ary_time_test ./n_ary_huffman < n_ary_huffman.c

small_time_test: small_compression
	time --verbose --output=small_time_test ./small_compression

CFLAGS += -Wall

.PHONY: clean
clean:
	rm -fv -- small_compression
	rm -fv -- n_ary_huffman

#

