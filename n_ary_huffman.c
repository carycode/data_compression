
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

// FIXME: support much larger numbers of symbols
// than 256 'char'.
void
histogram(
    const char * text,
    const int text_symbols,
    int h[text_symbols]
){
    /*
    I wish
    int h[text_symbols] = {0};
    zeroed out the array,
    but instead the compiler tells me
    "error: variable-sized object may not be initialized".
    */
    for(int i=0; i<text_symbols; i++){
        h[text_symbols] = 0;
    };
    const unsigned char * c = (const unsigned char *) text;
    /*
    Currently assumes there is no '\0' chars
    in the text block.
    */
    while( *c ){
        h[*c]++;
        c++;
    };
    // return h;
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

/*
How to fairly compare trinary Huffman to binary Huffman?
There are some cases where trinary Huffman
should give *better* compression than binary,
and vice versa.
In particular,
trinary should be best for files where letter frequencies
are 1/3, 1/9, 1/27, 81, 243, or etc.
(even after the overhead of converting trinary
back to binary and storing in a binary computer file).
Perhaps measure both ways:

Store trinary into binary file:
grab 5 trits at a time,
convert into a number 1..243,
and store as an 8-bit octet (which never uses byte 0 or 244..255).

Store binary into trinary file:
grab 3 bits at a time,
convert into a number 0..7,
and store as a 2-trit value 0..8.
*/

/*
FIXME: often a block of text
will happen to never use some particular
letter or punctuation (say, the exclamation mark).
Clearly we won't know this
until *after* we count the symbol frequencies.
Should *this* "huffman()" routine
expect some of the symbol frequencies to be zero,
or should we pre-filter them out
somewhere else
so this "huffman()" routine
only sees the *actual* symbols in use
and all the symbol_frequencies are positive?
*/

int * huffman (
    const int * symbol_frequencies,
    const int text_symbols,
    const int compressed_symbols
){
    /*
    I wish
    node list[text_symbols] = {0}; // list of both leaf and internal nodes
    int sorted_index[2*text_symbols] = {0};
    zeroed out the array,
    but instead the compiler tells me
    "error: variable-sized object may not be initialized".
    */
    node list[text_symbols];
    int sorted_index[2*text_symbols];
    // initialize the leaf nodes
    // (typically including the 256 possible literal byte values)
    for( int i=0; i<text_symbols; i++){
        list[i].leaf = true;
        list[i].leaf_value = i;
        list[i].count = symbol_frequencies[i];
        sorted_index[i] = i;
    };
    for( int i=text_symbols; i<2*text_symbols; i++){
        sorted_index[i] = 0;
    };

    // count out zero-frequency symbols
    int nonzero_text_symbols = 0;
    for( int i=0; i<text_symbols; i++ ){
        if( 0 != list[ sorted_index[ i ] ].count ){
            nonzero_text_symbols++ ;
        };
    };
    // FIXME: squeeze out zero-frequency symbols?

    int dummy_nodes = (nonzero_text_symbols - 1) % (compressed_symbols - 1);
    
    if( 2 == compressed_symbols ){
        //binary
        assert( 0 == dummy_nodes );
    };
    if( 3 == compressed_symbols ){
        //trinary
        const int expected_dummy = 1 - (nonzero_text_symbols & 1);
        assert( expected_dummy == dummy_nodes );
    };
    assert( dummy_nodes < (compressed_symbols - 1) );

    partial_sort( sorted_index, text_symbols, list );


    int lengths[nonzero_text_symbols] = {};
    // FIXME:

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

