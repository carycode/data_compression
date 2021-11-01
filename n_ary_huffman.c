
/* n_ary_huffman.c
WARNING: version  2021.00.01-alpha : extremely rough draft.
2021-10-25: started by David Cary

A few experiments with
n-ary Huffman compression.

Many Huffman data compression algorithms
2 output symbols (binary)
and
around 257 input symbols.
(Often
256 symbols: all possible bytes
DEFLATE has 288 symbols in its main Huffman tree:
0..255: all possible literal bytes 0-255
256: end-of-block symbol
257-285: match lengths
286, 287: not used, reserved and illegal.
DEFLATE has 32 symbols in its "distance" tree.
)

General Huffman
also covers other data compression algorithms
with other numbers of output symbols
and input symbols,
always emitting an integer number of output symbols
for each input symbol.

Here I experiment with
n=3 (trinary)
n=9 (nonary ?)
n=10 (decimal ?).

It also supports
n=2 (binary)
in order to make testing and comparison easier
with other Huffman-related software.

(With English text,
I expect n=9 and n=10
to act similar to
the straddling checkerboard cipher
).

What is a good way to make this
"unix pipeline friendly"
so I can pipe text through this?
Rather than raw 8-bit bytes.
For starters,
perhaps an option
to generate text
in the format of
lines of (for binary)
base64url digits
or
base16 digits
or (for trinary)
base3 digits
or
base27 digits
or (for senary)
base6 digits
or
base36 digits
broken by (to-be-ignored)
linefeeds every 70 characters or so.

*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h> // for bool, true, false
#include <iso646.h> // for bitand, bitor, not, xor, etc.
#include <assert.h> // for assert()
#include <ctype.h> // for isprint()

#define COMPILE_TIME_ASSERT(pred) switch(0){case 0:case pred:;}

int * histogram( const char * text, const int text_symbols ){
    int h[text_symbols] = {0};
    return h;
}

typedef struct {
    bool leaf;
    int count;
    // only if leaf=false, i.e., this node is internal
    int left_index;
    int right_index;
    // only if leaf=true, i.e., this node is a leaf:
    char leaf_value;
    // FUTURE: make this a union?
} node;

/*
it wouldn't hurt to completely sort,
but we really only require
the two smallest values.
Some unnecessarily complex "cleverness":
* perhaps do 2 passes of bubble sort
towards the beginning of the array,
to make sure the 2 smallest values
are at the start.
* perhaps do the initial sections
of a quicksort,
but rather than sorting the entire array,
each time we break the array
into 2 parts at the pivot,
only sort the left half of the array.
* Because we merge 2 nodes at the beginning,
consider allowing a few empty unused slots
near the beginning (rather than moving
*every* item in the sorted list down by 1
when the merged node ends up at the beginning,
or down by 2 when the merged node ends up at the end).
* because often the "merged" node
ends up at beginning or end of the sorted list
(or *near* one end),
consider trying to keep some empty slots
at both ends,
and inserting the new "merged" node
at the "best" end --
the end that requires the least shuffling of other nodes.
* Rather than a linear list, perhaps a priority heap?
*/
void partial_sort(){
}

int * huffman ( int * symbol_frequencies, int text_symbols, int compressed_symbols ){
    node list[text_symbols] = {0}; // list of both leaf and internal nodes
    int sorted_indexes[2*text_symbols] = {0};
    // initialize the leaf nodes
    // (typically including the 256 possible literal byte values)
    for( i=0; i<text_symbols; i++){
        list[i].leaf = true;
        list[i].leaf_value = i;
        list[i].count = symbol_frequencies[i];
        sorted_indexes[i] = i;
    };

    partial_sort( sorted_indexes, text_symbols, list );


    return lengths;
}
/*
support streaming:
break up input into
reasonably-sized blocks,
compress each block independently.
FUTURE:
somehow "optimize" exactly
where we break into blocks --
ideally if we have
regions of, say,
(a)typical English text,
(b)all-uppercase text,
(c)base-64 encoded random-looking data,
we would recognize
and break so each block
had only one kind of region.
FUTURE:
Consider remembering the last 2 or so
Huffman tables,
and rather than
emitting a full Huffman table
for each block,
instead emit a reference
indicating "same as #2 table".
FUTURE:
Consider somehow delta-compressing
Huffman tables,
so we can say something like
"same as #2 table except 'e' is 1 bit longer
and '\r' is 2 bits shorter."
Do we really need to specify *all* the changes,
or can we just specify
the "most impactful changes"
and somehow automatically adjust
the rest of the table,
reminiscent of the way
some implementations of length-limited Huffman
find the normal Huffman table
and then make adjustments?

FUTURE: support
history-limited compression,
(eventually) re-synchronizing
after the decoder is "turned on"
etc.
*/

int main(void){
    char * original_text = load_text();
    size_t original_length = strlen( original_text );
    // FIXME: support arbitrary number of symbols.
    int text_symbols = 258;
    int symbol_frequencies * = histogram( text, text_symbols );
    int compressed_symbols = 3; // 2 for binary, 3 for trinary, etc.
    // FUTURE: length-limited Huffman?
    int * canonical_lengths = huffman( symbol_frequencies, text_symbols, compressed_symbols);
    debug_print_table( canonical_lengths , text_symbols, compressed_symbols );
    char * compressed_text = compress( canonical_lengths, text_symbols, compressed_symbols);
    char * decompressed_text = decompress( compressed_text );
    size_t decompressed_length = strlen( decompressed_text );
    assert( original_length == decompressed_length );
    if( memcmp( original_text, decompressed_text, original_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    }
    
    return 0;
}

/* vim: set shiftwidth=4 expandtab ignorecase smartcase incsearch softtabstop=4 background=dark : */

