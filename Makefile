
# Makefile for compression routines
# 2015-05-24: started by David Cary


# C compiler options recommended by
# https://software.codidact.com/posts/282565
CFLAGS := $(CFLAGS) -std=c17 -pedantic-errors -Wall -Wextra -Werror -Wunused-result

# optional:
# warn on excessively large VLAs;
# DAV first heard of this warning at
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=98217
CFLAGS := $(CFLAGS) -Wvla-larger-than=0

# FUTURE:
# perhaps add the C compiler options
# and other static and dynamic tools
# mentioned by
# https://invisible-island.net/personal/lint-tools.html

default: time_test

time_test: \
	huffman_time_test \
	nybble_time_test \
	small_time_test \
#

huffman_time_test: n_ary_huffman
	time --verbose --output=huffman_time_test ./n_ary_huffman < n_ary_huffman.c

nybble_time_test: nybble_compression
	time --verbose --output=nybble_time_test ./nybble_compression

small_time_test: small_compression
	time --verbose --output=small_time_test ./small_compression

CFLAGS += -Wall
# FUTURE:
# consider using
# https://hackaday.com/2021/11/08/linux-fu-automatic-header-file-generation/

.PHONY: clean
clean:
	rm -fv -- small_compression
	rm -fv -- n_ary_huffman
	rm -fv -- nybble_compression
	rm -fv -- small_time_test
	rm -fv -- huffman_time_test
	rm -fv -- nybble_time_test

#

