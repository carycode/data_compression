
/* n_ary_huffman.c
WARNING: version  0.2021.00.01-alpha : extremely rough draft.
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
base9 digits
or
base27 digits
or
base81 digits
or (for decimal)
base-10 digits
or (for senary)
base6 digits
or
base36 digits
broken by (to-be-ignored)
linefeeds every 70 characters or so.

Perhaps reserve lines that begin with "#"
as internal debugging comments,
intended to be ignored
by other programs downstream in the pipeline.

FIXME:
Add a
"Quick Start Recipe"
analogous to the one at
https://github.com/ubf/ubf
to my README file.

FUTURE: length-limited Huffman?

FUTURE: use size_t rather than int for array lengths?

Future:
specify integer sizes
using the standard types
defined in stdint.h
INT_FAST8_MIN <= int_fast8_t <= INT_FAST8_MAX
INT_FAST16_MIN <= int_fast16_t <= INT_FAST16_MAX
0 <= uint_fast16_t <= UINT_FAST16_MAX
Unclear if I should directly include stdint.h,
or if I should only include
inttypes.h
(which in turn includes <stdint.h>, which in turn includes <limits.h>
).

Future: perhaps
<q>
Of the built-in integer types only use char, int, uint8_t, int8_t,
uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t,
uintmax_t, intmax_t, size_t, ssize_t, uintptr_t, intptr_t, and
ptrdiff_t.
...
printf format placeholders ...
size_t %zu (unsigned) ...
ssize_t %zd (signed)
</q>
--
as recommended by
https://neovim.io/doc/user/dev_style.html


Source code style:
DAV is a big fan of
4-space indents (no tabs)
(as per
"PEP 8 – Style Guide for Python Code"
https://peps.python.org/pep-0008/
)
and
else with ears ("}else{").
Consider *looking* at the recommendations
of a pretty printer / beautifier tool like "indent",
but don't blindly apply it
(as per
https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/
which recommends
"Avoid using the indent command as a last step; it can undo any manual beautification."
).


FUTURE:
consider using length-limited Huffman codes,
there's a nice description of
several length-limited Huffman algorithms,
including
the package-merge algorithm,
in
"Length-Limited Prefix Codes"
by Stephan Brumme
https://create.stephan-brumme.com/length-limited-prefix-codes/
Another description of the package-merge algorithm at
"Length-Limited Huffman Codes: How to create Huffman codes under a size constraint"
by Hayden Sather
https://experiencestack.co/length-limited-huffman-codes-21971f021d43
2022

FUTURE:
consider using the "shared interface"
described in
"Length-Limited Prefix Codes"
by Stephan Brumme
https://create.stephan-brumme.com/length-limited-prefix-codes/


FUTURE:
perhaps break up this monolithic file
into 2 or more separate files:
* decompression-only library
* compression-only library
* usage example, which also stress-tests those libraries
FIXME: explicitly mark functions only used internally with "static".

FUTURE:
make this an easily-usable library;
follow tips from
"Library Writing Realizations"
http://cbloomrants.blogspot.com/2015/09/library-writing-realizations.html


Currently this implementation
never uses dynamic allocation.
Or C99-style "variable-length arrays".
(Except we do use "length, table[length]" in the parameter list of some functions).
So we never have to worry about
garbage collection / memory fragmentation
memory leakage / etc.
If you do use dynamic allocation in the future,
consider continuing to avoid malloc()
and instead using things like calloc()
which ensure the new memory block is initialized.

Currently
* this implementation never uses "typedef"
* this implementation is careful to use "char" only for
letters of text, and not assume it's signed or unsigned.

FUTURE:
pick a version numbering system:
perhaps initially the light-hearted ZeroVer (0ver)
ZeroVer
https://sedimental.org/open_source_projects.html#zerover
https://0ver.org/
then later look at
I suspect I'll end up with
the slightly more serious
CalVer
https://sedimental.org/calver.html
https://sedimental.org/open_source_projects.html#calver-calendar-versioning
recommended by
"Designing a version"
https://sedimental.org/designing_a_version.html

DAV:
I expect to end up with *two* version numbers
associated with this software:
(1) The most important: a version number
in the header of the output file / stream
that helps recievers know (a) if
this distant-future stored-data format
has breaking changes and cannot be decoded
with this version of the reciever, and
(b) if this current, near-future, or past stored-data format
*can* be decoded with this version of the reciever,
and if so, which *particular* current or past version
format should be assumed.
(2) (less important) a version number
in the source code and in the debug output strings
of the executable.
Like most data compression algorithms,
I expect to go through many iterations of optimization
on the *compressor* and *decompressor*,
such that *all* the compressors that produce v7 files
and *all* the decompressors that can consume v7 files
can all interoperate with each other.

DAV:
If the version number is a *date*,
I may end up with *3* dates in the output file:
* the date/version the stored-data format was frozen
* the date/version the executable was "released",
the one that created this file.
* the date this output file was created.


FIXME:
tweak interface to comply with standard
data compression / decompression interface.
But which one?
Perhaps:
Ross Williams: the "V2" interface ("old_dc_stan_stream")
Ross Williams: notes on V2, and ideas for generalizing for
error correction, encryption, etc. ("old_dc_stan_just")
http://ross.net/compression/interface.html
or perhaps
"Coroutines in C"
by Simon Tatham
https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
as used by
libb64
https://sming.readthedocs.io/en/latest/_inc/Sming/Components/libb64/

FIXME:
I want this library to be able
to be used in
an arbitrarily long-running pipeline
and with arbitrarily-large files;
that means that we don't have room
to read *all* the data
before processing it,
we must read a little at a time.

FUTURE:
Should the decompressor
"work correctly" with concatenated files?
(see
https://en.wikibooks.org/wiki/Data_Compression/Streaming_Compression
).

FIXME:
avoid defining buffers with random "+1" sizes,
and instead follow the recommendations at
https://embeddedgurus.com/stack-overflow/2011/03/the-n_elements-macro/
.

FUTURE: benchmark this compressor.
* "Tips for benchmarking a compressor"
https://cbloomrants.blogspot.com/2016/05/tips-for-benchmarking-compressor.html
* "Data Compression/Evaluating Compression Effectiveness"
https://en.wikibooks.org/wiki/Data_Compression/Evaluating_Compression_Effectiveness
* "Benchmark files"
https://en.wikibooks.org/wiki/Data_Compression/References#Benchmark_files


I attempted to make this compressor use a
more-or-less human-readable output format:
* lines starting with '#' are comments
* whitespace is also optionally added for human readability
* base64_url, hexadecimal, decimal, etc. digits
on non-comment lines contain the compressed data.
* ... netstring ... JSON ... ?

*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h> // for bool, true, false
#include <iso646.h> // for bitand, bitor, not, xor, etc.
#include <assert.h> // for assert()
#include <ctype.h> // for isprint()
#include <limits.h> // for INT_MAX

#define COMPILE_TIME_ASSERT(pred) switch(0){case 0:case pred:;}
/*
FUTURE:
How should I spell the compile-time check?
https://en.wikibooks.org/wiki/C_Programming/Preprocessor_directives_and_macros#Compile-time_assertions
spells it COMPILE_TIME_ASSERT() .
In C++, it's spelled static_assert() .
Gnu C
https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Static-Assertions.html
seems to spell it as _Static_assert() .
The Loki library
https://stackoverflow.com/questions/31926676/where-is-static-assert-implemented
apparently spells it as STATIC_CHECK() .
*/
/*
FUTURE:
Use (compile-time check) where possible.
Where possible, replace run-time asserts with compile-time asserts.
Continue to use assert() (run-time check)
for things that cannot be checked at compile-time.
*/

/*
FUTURE:
consider using
https://en.wikipedia.org/wiki/C_data_types#Fixed-width_integer_types
*/

/*
NUM_ELEM finds the number
of elements of an array defined in the *same* function.
Alas,
it doesn't work inside
a function that accepts an array parameter
, so
a function that accepts an array parameter
should also accept a length-of-that-array parameter.
see
https://en.wikibooks.org/wiki/C_Programming/Pointers_and_arrays
for detils and limitations.
*/
#define NUM_ELEM(x) (sizeof (x) / sizeof (*(x)))

// integer min and max apparently not yet in standard libraries
// https://stackoverflow.com/questions/3437404/min-and-max-in-c
// although some people claim it is in stdlib.h
// http://tigcc.ticalc.org/doc/stdlib.html#min
static int
imax( int a, int b ){
    return ((a<b)? b : a);
}
static int
imin( int a, int b){
    return ((a<b)? a : b);
}


/* base64url (RFC 4648) */
/* used for binary Huffman
when "human-readable" output is selected.
*/
static char
int2digit(int i){
    const char
    base64url_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_";

    /* Ascii85 (RFC1924 "joke RFC") */
    /* git uses RFC1924 Ascii84:
    https://github.com/git/git/blob/master/base85.c
    */
#if (0)
    const char
    ascii85_table[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "!#$%&()*+-"
    ";<=>?@^_`{"
    "|}~";
#endif
    /* Z85 encoding https://rfc.zeromq.org/spec/32/ */
#if (0)
    const char
    z85_table[] =
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    ".-:+=^!/"
    "*?&<>()[]{"
    "}@%$#";
#endif
    /*
    The special case of
    encoding
    2 base-9 digits, or
    4 base-3 digits,
    per compressed output character.
    uses the first 81 characters
    in this Z85 table.
    (The Ascii85 table would work just as well).
    They use only ASCII isgraph() characters
    and avoids awkward characters
    double-quote, backslash, and space.
    "there are just 94 ASCII characters
    (excluding control characters, space, and del)"
    -- RFC1924

    2 base-6 digits in one char requires 36 chars
    3 base-6 digits in one char requires 216 chars
    */

    assert(0 <= i);
    assert(i < 64);
    return( base64url_table[i] );
}

int
digit2int(char input_digit){
    static char r[128] = {0};
    if( 0 == r[0] ){
        // one-time initialization
        for(int i=0; i<128; i++){
            r[i] = -1;
        };
        for(int i=0; i<64; i++){
            char digit = int2digit(i);
            int d = digit;
            r[d] = i;
        };
        // also support other base64url sets
        r['+'] = 62; // RFC 4648 standard
        r['/'] = 63; // RFC 4648 standard
        r['-'] = 62; // RFC 4648 base64url
        r['_'] = 63; // RFC 4648 base64url
    };
    assert( 0 < input_digit );
    unsigned char d = input_digit;
    assert( d < 128 );
    int result = r[d];
    assert( -1 != result );
    return(result);
}



// FIXME: support much larger numbers of symbols
// than 256 'char'.
void
histogram(
    const char * text,
    const int max_symbol_value,
    int h[max_symbol_value+1] // output-only
){
    /*
    I wish
    int h[text_symbols] = {0};
    zeroed out the array,
    but instead the compiler tells me
    "error: variable-sized object may not be initialized".
    */
    for(int i=0; i<(max_symbol_value+1); i++){
        h[i] = 0;
    };
    const unsigned char * c = (const unsigned char *) text;
    /*
    Currently assumes there is no '\0' chars
    in the text block.
    */
    while( *c ){
        assert( 0 < *c );
        assert( *c <= max_symbol_value );
        if( 126 < *c ){
            printf("# value above 126 near %s\n", c - 20);
        };
        h[*c]++;
        c++;
    };
    // return h;
    assert( 0 == h[258] );
}

/* should I use
typedef struct {
here?
*/
struct node{
    bool leaf;
    int count;
    // only if leaf=false, i.e., this node is internal
    // only for debugging binary trees -- doesn't work for trinary?
    int left_index;
    int right_index;
    // only if leaf=true, i.e., this node is a leaf:
    int leaf_value;
    // FUTURE: make this a union?
    // FIXME: int depth; // only used for length-limited Huffman
    int parent_index; // for trinary, decimal, etc. trees ?
    // The true parent index should never be 0,
    // since index 0 should always be a *literal* leaf,
    // and the true parent of *any* node
    // will always be a non-leaf.
    // We often initialize the parent index to 0
    // just to indicate that it isn't known yet.
    // int length; // only for debugging: FIXME: remove.
    // volume is only for debugging.
    int volume; // total bits for just this subtree.
    // volume of the root node:
    // root_node.volume = 0: still unknown
    // root_node.volume anything else:
    // the total bits in the compressed body,
    // not including the huffman table prefix header.
    // volume of leaf nodes: 0 (because it makes the math work out)
    // volume of 1st interior node pointing only to leaf nodes:
    // for this subtree alone, each leaf uses 1 bit / trit / etc.,
    // so the 1st node volume = sum( count( each leaf ) )
    // for general interior node:
    // volume = sum( count(each child) ) + sum( volume(each child) ).
};

void
debug_print_node(struct node n, int index){
    char c = n.leaf_value;
    if(!isprint(c)){
        c = 0;
    };
    printf("# %i { %i, count:%i, ... '%c', parent:%i }\n",
        index, n.leaf, n.count, c, n.parent_index
        );
}

void
debug_print_node_list(
    const int list_length,
    const struct node list[list_length]
){
    printf("# list_length: %i\n", list_length);
    for(int i=0; i<list_length; i++){
        bool nonzero = (0 != list[i].count);
        bool nonleaf = !(list[i].leaf);
        if(nonzero or nonleaf){
            debug_print_node( list[i], i );
        };
    };
}

/*
it wouldn't hurt to completely sort,
but we really only require
the two smallest values
for binary Huffman.
(The 3 smallest values for trinary Huffman,
The 10 smallest values for decimal Huffman,
...
The n smallest values for n compressed symbols.
).
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
* FIXME: when merging,
make sure
(if there is an option -- sometimes there is no option)
we always
pick the N that *minimizes*
the longest Huffman code.
For example, if we have frequencies of
1, 1, 2, 2,
and if we have N=2 (binary Huffman),
prefer to make the tree
            6
         /     \
        2       4
       / \     / \
      1   1   2   2
      (depth = longest Huffman code length=2)
rather than
            6
           / \
          4   2
         / \
        2   2
       / \
      1   1
      (depth = longest Huffman code length = 3)
.
*Both* of these trees
can be converted to (different) canonical Huffman codes.
Both of these trees give exactly the same
total compressed bitlength compressed payload.
I suspect (?) that
the minimum-depth tree
will make decompressing a little faster
and
perhaps (?)
the minimum-depth tree
will make the header
(representing the canonical Huffman tree)
at the beginning of the compressed block
a little shorter.
* Rather than a linear list, perhaps a priority heap?
https://en.wikipedia.org/wiki/Partial_sorting
* FUTURE: some faster sorted queue implementation?
*/
static
void
print_counts(
    const int list_length,
    const int sorted_index[list_length],
    const struct node list[list_length],
    const int min_active_node,
    const int max_node
){
    for(int i=min_active_node; i<(max_node+1); i++){
        int count_one = list[ sorted_index[ i ] ].count;
        printf("# %i\n", count_one);
    };
}
static
bool
swap_if_necessary(
    const int list_length,
    int sorted_index[list_length], // modified
    struct node list[list_length], // input-only
    const int left,
    const int right
){
    int count_one = list[ sorted_index[ left  ] ].count;
    int count_two = list[ sorted_index[ right ] ].count;
    if( count_two < count_one ){
        int c = sorted_index[ left ];
        int d = sorted_index[ right ];
        sorted_index[ left ] = d;
        sorted_index[ right ] = c;
        return true;
    };
    return false;
}
static
void
partial_sort(
    const int list_length,
    int sorted_index[list_length], // modified
    struct node list[list_length], // input-only
    const int min_active_node,
    const int max_node
){
    // shuffle the indexes in the sorted_index[]
    // so that, for example,
    // when 3=compressed_symbols,
    // the 3 counts
    //  list[sorted_index[min_active_node].count
    //  list[sorted_index[min_active_node+1].count
    //  list[sorted_index[min_active_node+2].count
    // are the *smallest* counts in the whole range.
    // FIXME:
    /*
    print_counts(
        list_length,
        sorted_index,
        list,
        min_active_node,
        max_node
    );
    */
    printf("# sorting %i items...\n", max_node - min_active_node + 1);
    assert( min_active_node < list_length );
    assert( min_active_node < max_node );
    int total_swapped = 0;
    do{
        total_swapped = 0;
        for(int i=(max_node-1); i>=(min_active_node); i--){
            bool swapped = swap_if_necessary(
                list_length,
                sorted_index,
                list,
                i,
                i+1
            );
            total_swapped += swapped;
        };
        /*
        if(total_swapped){
            printf("# found %i out-of-order; rescanning...",
                total_swapped
            );
        };
        */
    }while(total_swapped);
    print_counts(
        list_length,
        sorted_index,
        list,
        min_active_node,
        max_node
    );
    printf("# ... sorted.\n");
}

/*
How to fairly compare trinary Huffman to binary Huffman?
There are some cases where trinary Huffman
should give *better* compression than binary,
and vice versa.
In particular,
trinary should be best for files where letter frequencies
are 1/3, 1/9, 1/27, 1/81, 1/243, or etc.
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
FIXME:
if we always allocate node list[] with calloc(),
can we skip this initialization?
*/
void
setup_nodes(
    const int list_length,
    struct node list[list_length],
    // int sorted_index[list_length],
    const int max_leaf_value,
    const int symbol_frequencies[max_leaf_value+1]
){
    printf("# starting setup_nodes.\n");
    printf("# list_length:%i\n", list_length);
    printf("# max_leaf_value:%i\n", max_leaf_value );
    // initialize the leaf nodes
    // (typically including the 256 possible literal byte values)
    assert( 0 == symbol_frequencies[258] );
    assert( (max_leaf_value+1) < list_length );
    for( int i=0; i<(max_leaf_value+1); i++){
        list[i].leaf = true;
        list[i].leaf_value = i;
        assert( 0 <= list[i].leaf_value );
        list[i].count = symbol_frequencies[i];
        assert( 0 <= symbol_frequencies[i] );
        // sorted_index[i] = i;
        list[i].parent_index = 0; // will be filled in later
        // zero out stuff that only applies to non-leaves
        list[i].left_index = 0;
        list[i].right_index = 0;
    };
    for( int i=max_leaf_value+1; i<list_length; i++){
        list[i].leaf = false;
        // sorted_index[i] = 0;
        list[i].left_index = 0; // will be filled in later
        list[i].right_index = 0; // will be filled in later
        list[i].parent_index = 0; // will be filled in later
        list[i].count = 0; // will be filled in later by the algorithm
        // zero out stuff that only applies to leaves
        list[i].leaf_value = 0;
    };
    printf("# Done setup_nodes.\n");
    assert( true == list[max_leaf_value].leaf );
    assert( false == list[max_leaf_value+1].leaf );
    assert(0 == list[259].count );
    debug_print_node_list(list_length, list);
    assert( 0 == list[258].count );
    assert( 0 == list[300].count );
}

/*

DAV:
As David Huffman pointed out,
if we have 3 or more output symbols
(i.e., trinary Huffman compression or n-ary Huffman compression)
we may need dummy symbols.

DAV:
Perhaps the simplest case where it's clear we need dummy symbols:
If we have
3 compressed symbols (trinary Huffman)
and we have 4 unique input symbols,
if we merge the 3 lowest-frequency symbols
we always (no matter the input symbol counts) get
a tree with this shape:

         root  (bad: not optimum!)
       /   |  \
    / | \  d  (unused)
    a b  c

It's always better to use
a tree with this shape:

         root  (good! optimum shaped tree)
       /   |  \
    / | \  c   d
    a b (dummy unused)

Another example:

With 4-ary Huffman,
if we have 5 unique input symbols we need 2 dummy symbols:

                     root (good! optimum shaped tree).
                  -------
                 /  | |  \
          -------   c d   e
         /  | |  \
    (dummy) a b (dummy)

FUTURE:
consider posting what I've learned about ternary and general n-ary Huffman
to
https://cs.stackexchange.com/questions/40833/the-algorithm-yields-optimal-ternary-codes

*/

void
generate_huffman_tree(
    // const int text_symbols,
    const int list_length,
    struct node list[list_length], // in-out: updated
    const int compressed_symbols, // 3 for trinary
    const int max_leaf_value
){
    assert( 0 == list[0].count ); // FIXME: can't handle NULL bytes yet.
    assert( 0 == list[258].count );
    assert(0 == list[259].count );
    // count out zero-frequency symbols
    int nonzero_text_symbols = 0;
    for( int i=0; i<(max_leaf_value+1); i++ ){
        // if( 0 != list[ sorted_index[ i ] ].count ){
        if( 0 != list[ i ].count ){
            nonzero_text_symbols++ ;
        };
    };
    printf("# found %i unique symbols actually used.\n", nonzero_text_symbols);
    // FIXME: squeeze out zero-frequency symbols?

    // setup internal sorted_index
    int sorted_index[list_length];
    assert( max_leaf_value < list_length );
    for( int i=0; i<(max_leaf_value+1); i++){
        sorted_index[i] = i;
    };
    for( int i=(max_leaf_value+1); i<list_length; i++){
        sorted_index[i] = i;
    };

    int dummy_nodes =
        (compressed_symbols - 1) -
        ((nonzero_text_symbols - 1) %
        (compressed_symbols - 1));
    
    printf("# %d : compressed symbols\n", compressed_symbols );
    if( 2 == compressed_symbols ){
        //binary
        assert( 0 == dummy_nodes );
    };
    if( 3 == compressed_symbols ){
        //trinary
        const int expected_dummy = 1 - (nonzero_text_symbols & 1);
        printf("nonzero_text_symbols: %i\n", nonzero_text_symbols);
        printf("compressed_symbols: %i\n", compressed_symbols);
        printf("dummy_nodes: %i\n", dummy_nodes);
        assert( expected_dummy == dummy_nodes );
    };
    assert( dummy_nodes < (compressed_symbols - 1) );
    printf("# using %i dummy nodes.\n", dummy_nodes);
    printf("# max_leaf_value: %i\n", max_leaf_value);
    for(int i=(max_leaf_value+1); i<(max_leaf_value + 1 + dummy_nodes); i++){
        sorted_index[i] = i;
        list[i].count = 1; // minimum count for dummy nodes.
        // FUTURE: perhaps
        // force dummy nodes have a nonzero count
        // far *less* than any real node,
        // perhaps by scaling all the real node counts
        // by 2 or 3 or 10 or so.
    };
    assert(1 == list[259].count ); // dummy node
    assert( 1 == ((nonzero_text_symbols + dummy_nodes) % (compressed_symbols - 1)) );
    // ZQ

    int min_active_node = 0;
    // when leaves are "merged" together to some internal node,
    // list[n] will be that internal node.
    int max_active_node = max_leaf_value + dummy_nodes;
    assert( 0 == list[258].count );
    /*
    debug_print_node_list(list_length, list);
    */
    // squeeze out zero values
    do{
        printf("# squeezing out zero counts.\n");
        while( 0 == list[sorted_index[min_active_node]].count ){
            min_active_node++;
        };
        partial_sort(
            list_length, sorted_index, list,
            min_active_node,
            max_active_node
        );
    }while( 0 == list[sorted_index[min_active_node]].count );
    // check that there are no zero values
    for(int i=min_active_node; i<(max_active_node+1); i++){
        assert( 0 != list[sorted_index[i]].count );
    };
    printf("# No more zero counts.\n");
    /*
    debug_print_node_list(list_length, list);
    */
    while( min_active_node < max_active_node ){
        const int n = max_active_node+1;
        printf("# n=%i\n", n);
        assert(0 == list[n].count);
        assert( n < list_length );
        // find the lowest-frequency (other than 0) nodes,
        // merge together to produce a non-leaf internal node.
        // Repeat until only one node.
        partial_sort(
            list_length, sorted_index, list,
            min_active_node,
            max_active_node
        );
        
        assert( false == list[n].leaf );
        if(1){ // not really used.
            list[n].left_index = sorted_index[min_active_node];
            list[n].right_index = sorted_index[min_active_node+1];
        };
        int parent_count = 0;
        for(int i=0; i<compressed_symbols; i++){
            int child_i = sorted_index[min_active_node];
            if( 0 == list[child_i].count ){
                /*
                debug_print_node_list(list_length, list);
                */
                for(int j=min_active_node; j<=max_active_node; j++){
                    printf("# odd: %i, %i\n", list[sorted_index[j]].count, sorted_index[j]);
                };
            };
            assert( 0 != list[child_i].count );
            list[child_i].parent_index = n;
            parent_count += list[child_i].count;
            min_active_node++;
        };
        list[n].count = parent_count;
        assert(0 != list[n].count);
        assert( n == sorted_index[n]);
        max_active_node++;
        assert( n == max_active_node );
    };
    printf("# finished tree.\n");
    assert( min_active_node == max_active_node );
}

/*
Generate a list of "lengths",
the lengths used in canonical Huffman tables.
The root node in the Huffman tree has length 0,
both (for binary)
or all 10 (for decimal) of the nodes directly "under" the root node
(i.e., that all point to the root node)
have length 1 digit,
the next row has length 2 digits, etc.

The node_list[] is in some arbitrary,
possibly very-mixed-up order.
The node corresponding to the letter 'b'
could be *any* index of the node_list.

The lengths[] contains only the
length of each leaf node,
in numerical order.
The (Huffman-compressed) length of the letter 'b'
is stored in lengths['b'].

FIXME:
always allocate lengths[] with calloc(),
so we can skip the zero initialization?

*/
void
summarize_tree_with_lengths(
    const int list_length,
    const struct node list[list_length], // input-only
    // int root_node_index,
    const int max_leaf_value,
    int lengths[max_leaf_value+1], // output-only
    const int leaves
){
    /*
    debug_print_node_list(list_length, list);
    */
    // zero out the lengths
    for( int i=0; i<(max_leaf_value+1); i++){
        lengths[i] = 0;
    };
    // All of the leaf nodes
    // should be the first text_symbols
    // at the beginning of the list[].
    // All the internal nodes
    // should immediately follow.
    // There may be a few dummy unused nodes (0 == list[x].count)
    // at the end of the list[].
    printf( "# leaves: %i\n", leaves);
    assert( list_length > leaves );
    assert( false == list[leaves].leaf );
    for( int i=0; i<leaves; i++){
        debug_print_node( list[i], i );

        assert( true == list[i].leaf );
        if( list[i].count ){
            assert( list[i].parent_index );
            // every leaf should have a parent
            // unless it is never used (i.e., 0 == count).
        };
        // scan up the parent_index up to the root
        int child = i;
        int sum = 0;
        do{
            sum++;
            int parent = list[child].parent_index;
            child = parent;
        }while( 0 != child );
        sum--; // don't count root node.
        // lengths[i] = sum;
        /*
        Originally "list[i].leaf_value" was a "char".
        On systems where "char" is a *signed* integer,
        this can set "leaf_value" to a *negative* number,
        then "lengths[leaf_value] = sum;"
        overwrites stuff it shouldn't.
        Then running this program gave the message
        "*** stack smashing detected ***: terminated"
        */
        int leaf_value = list[i].leaf_value;
        assert( 0 <= leaf_value );
        assert( leaf_value <= max_leaf_value );
        lengths[leaf_value] = sum;
    };
    printf("# finished summary.\n");
}

void
debug_print_table( int text_symbols, int canonical_lengths[text_symbols], int compressed_symbols ){
    // FIXME:
    printf( "# compressed_symbols: %i \n", compressed_symbols );
    printf( "# (2 === compressed symbols is the common binary case)\n" );
    printf( "# (3 === compressed symbols for trinary)\n" );
    printf( "# text_symbols: %i \n", text_symbols );
    printf( "# (typically text_symbols around 300, one for each byte and a few other special ones, even if most of those byte values never actually occur in the text) \n" );
    for( int i=0; i<text_symbols; i++ ){
        printf("# symbol %i : length %i ", i, canonical_lengths[i] );
        if( isprint( i ) ){
            printf("(%c)", (char)i );
        };
        printf("\n" );
    };
}

void test_summarize_tree_with_lengths(void){
    /* FIXME:
    int text_symbols = 300;
    */
    // const int text_symbols_doubled = 6;
#define text_symbols_doubled (6)
    const int text_symbols = (text_symbols_doubled) / 2;
    struct node list_a[text_symbols_doubled] = {
        { true, 9, 0, 0, 'a', 2, 0 },
        { true, 9, 0, 0, 'b', 2, 0 },
        { false, 4, 0, 1, 0, 0, 0 }
    };
    int leaves = 2;
    int max_leaf_value = 'z';
    int lengths_a[max_leaf_value+1];
    int list_length = text_symbols_doubled;
    summarize_tree_with_lengths( list_length, list_a, max_leaf_value, lengths_a, leaves );
    int compressed_symbols = 2;
    debug_print_table( max_leaf_value, lengths_a, compressed_symbols );
    assert( 1 == lengths_a['a'] );
    assert( 1 == lengths_a['b'] );
    printf("# Done with list_a test.\n");

#define list_b_length (5)
    struct node list_b[list_b_length] = {
        { true, 9, 0, 0, 'a', 4, 0 },
        { true, 9, 0, 0, 'b', 3, 0 },
        { true, 8, 0, 0, 'c', 3, 0 },
        { false, 17, 1, 2, 0, 4, 0 },
        { false, 26, 0, 3, 0, 0, 0 }
    };
    leaves = 3;
    max_leaf_value = 'z';
    int lengths_b[max_leaf_value+1];
    summarize_tree_with_lengths( list_b_length, list_b, max_leaf_value, lengths_b, leaves );
    assert( 1 == lengths_b['a'] );
    assert( 2 == lengths_b['b'] );
    assert( 2 == lengths_b['c'] );
    for( int i=0; i<text_symbols; i++ ){
        // something about shorter lengths having larger frequency counts
    };
    printf("# Done with list_b test.\n");
}

/*
Given a histogram of symbol frequencies,
generate the optimal length for each symbol
using the Huffman algorithm.
*/
void
huffman(
    const int max_leaf_value,
    const int symbol_frequencies[max_leaf_value+1],
    const int compressed_symbols,
    int lengths[max_leaf_value+1]
){
    const int list_length = (2*max_leaf_value);
    assert(1 < compressed_symbols);
    /*
    I wish
    node list[text_symbols] = {0}; // list of both leaf and internal nodes
    int sorted_index[2*text_symbols] = {0};
    zeroed out the array,
    but instead the compiler tells me
    "error: variable-sized object may not be initialized".
    */
    struct node list[list_length];
    // int sorted_index[max_leaf_value_doubled];

    setup_nodes(
        list_length, list,
        // sorted_index,
        max_leaf_value,
        symbol_frequencies
    );
    assert(0 == list[259].count );
    /*
    printf("# after initial setup_nodes: \n");
    debug_print_node_list(
        list_length, list
    );
    */

    assert( 0 == list[258].count );
    generate_huffman_tree(
        list_length,
        list,
        // sorted_index,
        compressed_symbols,
        max_leaf_value
    );

    printf("# summarizing tree...\n");
    printf("# max_leaf_value: %i\n", max_leaf_value);
    summarize_tree_with_lengths( list_length, list, max_leaf_value, lengths, max_leaf_value+1 );
    printf("# discarding tree, keeping only lengths.\n");
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
/*
... avoids using malloc() ...
*/

size_t
load_more_text(FILE * in, const size_t bufsize, char * buffer){
    if( ferror(in) ){
        // read error already occurred?
        printf("# previous read error?\n");
        return -1;
    };
    size_t ret_code = fread( buffer, 1, bufsize, in);
    if( bufsize == ret_code ){
        // read all bufsize characters successfully;
        // there's probably more characters later.
        printf("# successful full-buffer read\n");
        // Append a zero
        // to convert this to a valid C string.
        buffer[ret_code] = '\0';
        return ret_code;
    };

    // else did *not* read a full bufsize characters.
    if( feof(in) ){
        // hit end of file.
        printf("# successful part-buffer read (end-of-file)\n");
        // Append a zero
        // to convert this to a valid C string.
        buffer[ret_code] = '\0';
        return ret_code;

    }else if( ferror(in) ){
        // some kind of read error.
        printf("# new read error?\n");
        return -1;
    };
    assert( 0 /* "This should never happen." */ );
    return -1;
}

int
count_nonzero_items(
    const int array_length,
    const int a[array_length]
){
    int count = 0;
    for(int i=0; i<array_length; i++){
        bool is_zero = (0 == a[i]);
        if(!is_zero){
            count++;
        };
    };
    return count;
}

// Similar to
//   #include <math.h>
//   double pow(double base, double exponent);
// but with only integer values:
// inspired by
// https://stackoverflow.com/questions/213042/how-do-you-do-exponentiation-in-c
int power(const int base, const int exp){
    assert(0 <= exp);
    if(0 == exp){
        return 1;
    }else if(exp & 1){
        return base * power(base, exp - 1);
    }else{
        int temp = power(base, exp >> 1);
        return temp * temp;
    };
}

// find maximum value in array
int array_max(
        const int max_symbol_value,
        const int canonical_lengths[max_symbol_value+1]
    ){
    int max_canonical_length = 0;
    const int debug = 1;
    for(int i=0; i<max_symbol_value; i++){
        if(debug){
            if( canonical_lengths[i] < 0 ){
                printf(
                    "unexpected length: i = %i; canonical_length[i] = %i \n",
                    i, canonical_lengths[i]
                    );
            };
        };
        assert( canonical_lengths[i] >= 0 );
        max_canonical_length = imax(
                max_canonical_length,
                canonical_lengths[i]
                );
    }
    return max_canonical_length;
}
// find minimim non-zero value in array
int array_min(
        const int max_symbol_value,
        const int canonical_lengths[max_symbol_value+1]
    ){
    int min_canonical_length = 300;
    const int debug = 1;
    for(int i=0; i<max_symbol_value; i++){
        if(debug){
            if( canonical_lengths[i] < 0 ){
                printf(
                    "unexpected length: i = %i; canonical_length[i] = %i \n",
                    i, canonical_lengths[i]
                    );
            };
        };
        assert( canonical_lengths[i] >= 0 );
        // the min canonical length that is not zero.
        if(0 != canonical_lengths[i]){
            min_canonical_length = imin(
                min_canonical_length,
                canonical_lengths[i]
                );
        };
    }
    return min_canonical_length;
}


void
convert_lengths_to_encode_table(
        /* inputs */
        const int max_symbol_value, // input-only
        const int canonical_lengths[max_symbol_value+1], // input-only
        const int compressed_symbols, // input-only: 2 for binary, 3 for trinary, etc.
        /* outputs */
        int encode_length_table[max_symbol_value+1], // output-only
        unsigned int encode_value_table[max_symbol_value+1] // output-only
    ){
    // the canonical_lengths
    // and the encode_length_table
    // are in terms of output symbols --
    // i.e., a length of "3" means 3 digits,
    // and each digit may be a trinay digit (when compressed_symbols = 3)
    // or a bit (when compressed_symbols = 2)
    // or etc.
    const int debug = 1;
    if(debug){
        printf(
            "compressed_symbols: i = %i; max_symbol_value = %i \n",
            compressed_symbols,
            max_symbol_value
            );
    };

    assert(max_symbol_value);
    assert(compressed_symbols);
    const int max_canonical_length = array_max( max_symbol_value, canonical_lengths );
    const int min_canonical_length = array_min( max_symbol_value, canonical_lengths );
    // FUTURE: need to allow longer lengths
    // if we have huge Huffman tables.
    assert(max_canonical_length < 16);
    assert(0 < min_canonical_length);
    assert( min_canonical_length <= max_canonical_length );
    // FIXME: perhaps we shouldn't assume the NULL character never occurs.
    assert(0 == canonical_lengths[0]);

    // clear the symbol tables
    for(int i=0; i<max_symbol_value; i++){
        encode_length_table[i] = 0;
        encode_value_table[i] = 0;
    };
    // construct the canonical Huffman codes,
    // where
    // encode_length_table[i] == the number of output symbols to represent character i
    // encode_value_table[i] == the value to represent character i
    //
    //
    // The longest 2 codes in binary canonical Huffman
    // are always the same length.
    // (There may or may not be other codes just as long).
    // Since Huffman codes
    // always produce a "complete" binary tree,
    // there's always exactly
    // one leaf labeled with the all-ones code
    // and
    // one leaf labeled with the all-zeros code.
    //
    // A complete n_ary tree
    // with n = compressed_symbols
    // has 1 more than a multiple of (n-1) leaves.
    // Proof:
    // starting from a tree of 1 leaf,
    // any complete n_ary tree can be generated
    // by iteratively
    // picking some leaf and replacing that leaf
    // with an internal node
    // linked to n leaves, which gives
    // a net addition of (n-1) leaves.
    // David Huffman's original paper, if DAV remembers correctly,
    // points out that if we don't actually *have*
    // a full 1 + k*(n-1) symbols,
    // we may need some "dummy" leaves that don't really exist
    // equal in length to the least-frequent symbol that does exist.
    // The number of dummy symbols we need is d,
    // 0 <= d < (n-1).
    // So a binary tree never needs dummy symbols,
    // a trinary tree may need 1 dummy symbol (or may not need any),
    // a 5-ary tree may need 0 to 3 dummy symbols, etc.
    //
    // There seem to be 2 slightly different definitions:
    // (both have exactly the same length
    // for each and every code,
    // so make no difference in total compressed file):
    // Wikipedia: Canonical Huffman
    // has the all-ones code be the longest,
    // and the all-zeros code be the shortest.
    // Michael Schindler
    // http://www.compressconsult.com/huffman/#canonical
    // has the all-ones code be the shortest,
    // and the all-zeros code be the longest.
    //
    // For now I'm choosing
    // all-ones code as the longest
    // (this may change later).
    // In that case,
    // the longest 2 codes in binary canonical Huffman are
    // always
    // the all-ones code
    // and a code that is all-ones followed by a single 0.
    // FUTURE:
    // Consider alternate representation:
    // * two numbers that encode:
    // a bunch of '0' bits,
    // (for the all-zeros-code longest)
    // (or a count of '1' bits, for the all-ones-code longest)
    // followed by zero or more bits
    // starting with a '1' bit.
    // * for base-9 or base-10 Huffman compression,
    // two numbers that encode
    // a count of '0' digits,
    // followed by a number that
    // is either zero (i.e., the all-zeros symbol)
    // or, (when printed out in the appropriate base-9 or base-10),
    // starts with a non-zero digit 1, 2, 3, ... 8, ... etc.
    // that is the remaining digits of the Huffman code.
    // * for base-9 or base-10 Huffman compression,
    // two numbers that encode
    // a count of '0' digits,
    // and a number that
    // is either zero (i.e., the all-zeros symbol)
    // or when printed out in *hexadecimal*
    // starts with a non-zero digit 1, 2, 3, ... 8, ... etc.
    // that is the remaining digits of the Huffman code
    // (packed 4 bits per digit).
    //
    // The
    // http://www.compressconsult.com/huffman/#canonical
    // points out (with proof) that 
    // (with Schindler's definition of canonical Huffman)
    // "The first rule guarantees that no more than the ceil(log2(alphabetsize)) rightmost bits of the code can differ from zero - see below."
    // The
    // https://www.anaesthetist.com/mnm/compress/huffman/Findex.htm#index.htm
    // says the same thing
    // (and uses the same definition
    // with all-zeros being the longest code,
    // and all-ones as the shortest code).
    // With an alphabetsize of 512 symbols,
    // this implies that
    // no more than 9 rightmost bits of the code
    // can differ from zero;
    // so the code for each of those 512 symbols
    // can be represented by
    //      numzeros: the number of zeros at the beginning of the code
    //      (implicit 1 for every symbol that's not the all-zeros symmbol)
    //      rightmost_bits: up to 8 more arbitrary bits.
    //
    // Currently we scan through the table
    // a bunch of times --
    // at most max_canonical_length times.
    // There's likely a more efficient approach.
    #if(0)
        // start with longest code?
        int current_code = (1<<max_canonical_length) - 1;
        int current_length = max_canonical_length;
    #endif
    // start with shortest code:
    int current_code = 0;
    // int current_length = min_canonical_length;
    for( int current_length = min_canonical_length; current_length <= max_canonical_length; current_length++){
        const int debug = 1;
        if(debug){
            printf("current_length = %i.\n", current_length);
        };
        for(int i=0; i<=max_symbol_value; i++){
            // only look at symbols
            // that match current_length
            // on this pass:
            if( current_length == canonical_lengths[i] ){
                if(2 == compressed_symbols){
                assert( current_code < (1<<(current_length + 1)) );
                };
                assert( current_code < power(compressed_symbols, current_length+1) );
                encode_length_table[i] = current_length;
                encode_value_table[i] = current_code;
                if(debug){
                    printf("Assigning i=%i to code 0x%x == %i.\n", i, current_code, current_code);
                };
                current_code += 1; // increment code.
            };
        };
        if(0){ // only binary
        current_code <<= 1; // append zero bit and increment current_length
        };
        current_code *= compressed_symbols; // append zero symbol and increment current_length
    };
    // LSB is zero, take it back off.
    if( 2 == compressed_symbols ){
        assert( 0 == ( 1 & current_code ) );
    };
    assert( 0 == ( current_code % compressed_symbols ) );
    if(0){ // only binary
    current_code >>= 1;
    };
    current_code = current_code / compressed_symbols;
    // undo last increment
    current_code -= 1;
    const int max_actual_code = current_code;
    // assert last code assigned is the all-ones max-length code:
    // (or the all-max-digit max-length code)
    const int max_possible_code = power(compressed_symbols, max_canonical_length) - 1;
    const int dummy_symbols = max_possible_code - max_actual_code;
    if( debug && ( 2 == compressed_symbols )){
        assert( ((1<<max_canonical_length) - 1) == current_code );
        assert( 0 == dummy_symbols );
    };
    if( debug && ( 3 == compressed_symbols )){
        int nonzero_symbols = count_nonzero_items( max_symbol_value, canonical_lengths );
        printf( "nonzero symbols: %i\n", nonzero_symbols );
        printf( "max_canonical_length: %i\n", max_canonical_length );
        printf( "max_possible_code: %i\n", max_possible_code );
        printf( "max_actual_code: %i\n", max_actual_code );
        bool odd = (nonzero_symbols & 1);
        bool even = !odd;
        if(odd){
            assert( 0 == dummy_symbols );
        };
        if(even){
            assert( 1 == dummy_symbols );
        };
    };
    if(debug){
        printf( " compressed_symbols = %i, max_canonical_length = %i \n", compressed_symbols, max_canonical_length);
        printf( " compressed_symbols ** max_canonical_length - 1 = %i \n",  max_possible_code);
        printf( " max_actual_code = %i \n",  max_actual_code);
        printf( " dummy_symbols = %i \n",  dummy_symbols );
    }
    assert( 0 <= dummy_symbols );
    assert( dummy_symbols < (compressed_symbols - 1) );
}


/*
starts writing into compressed_text
at byte compressed_text[start],
last byte written is
at byte compressed_text[start + retval -1]. (???)
*/
int
represent_items_with_codes(
            const int max_symbol_value,
            int canonical_lengths[max_symbol_value+1],
            const int compressed_symbols, // 2 for binary, 3 for trinary, etc.
            const int bufsize, // const size_t bufsize,
            const int original_length,
            char original_text[bufsize+1],
            int start, // index to start writing in compressed_text
            char compressed_text[bufsize+1] // output
    ){

    unsigned int encode_value_table[max_symbol_value + 1];
    int encode_length_table[max_symbol_value + 1];
    assert(sizeof(encode_value_table) >= 16);
    // the convert_lengths_to_encode_table
    // has some endian-sensitive stuff
    // in the encode_value_table
    convert_lengths_to_encode_table(
        max_symbol_value,
        canonical_lengths,
        compressed_symbols, // 2 for binary, 3 for trinary, etc.
        encode_length_table,
        encode_value_table
        );
    // FUTURE: handle various other output formats
    // Currently using base64url.
    const int bits_per_output_symbol = 6; // base64url
    int bit_offset = 0;
    int char_offset = start;
    for(int i=0; i<original_length; i++){
        // FUTURE: consider re-ordering
        // to make more byte-aligned.
        // For now, completely bit-oriented.
        int item = original_text[i];
        int encoded_value = encode_value_table[item];
        int encoded_length = encode_length_table[item];
        assert(encoded_length > 0);
        assert(encoded_value < (1<<encoded_length));

        assert(0);

        bit_offset += encoded_length;
        while( bit_offset >= bits_per_output_symbol ){
            assert(bit_offset > 0);
            assert(0);
            assert(2 == compressed_symbols);
            const char letter = int2digit(encoded_value);
            assert(0);
            compressed_text[char_offset] = letter;
            char_offset++;
            bit_offset =- bits_per_output_symbol;
        };

    };

    return 32767;
}

/*
Given a list of lengths
(one length for each symbol)
and a block of uncompressed text,
generate a block of compressed text
(starting with a compact representation
of the canonical list of lengths).
*/
static void
compress(
    const int max_symbol_value,
    int canonical_lengths[max_symbol_value+1],
    const int compressed_symbols, // 2 for binary, 3 for trinary, etc.
    const int bufsize, // const size_t bufsize,
    const int original_length,
    char original_text[bufsize+1],
    char compressed_text[bufsize+1] // output
){
    assert( original_length <= bufsize );
    if(max_symbol_value > 1000){
        assert(0);
    };
    if( compressed_symbols < 2 ){
        assert(0); 
    };
    if( compressed_symbols > 2 ){
        printf("# %d : compressed_symbols.\n", compressed_symbols );
        // FIXME:
        printf("# header ....\n");
        // FIXME: perhaps make d a simple integer index *into* the compressed_text[] array
        char * d = compressed_text;
        char * type = "\nX"; // Huffman table type 1 (human-readable)
        // FUTURE: there's probably a better way
        // of encoding max_symbol_value and compressed_symbols.
        int s_length = 0;
        if(max_symbol_value < 10){
            s_length = 1;
        }else if(max_symbol_value < 100){
            s_length = 2;
        }else if(max_symbol_value < 1000){
            s_length = 3;
        };
        assert(0 != s_length); // FUTURE: handle more than 1000 symbols.
        const int netstring_length =
            strlen(type) + s_length + 1 + max_symbol_value + 1;
        printf("# netstring_length: %d\n", netstring_length);
        d +=
        sprintf(d, "%d:", // start of netstring
            netstring_length
            );
        int start_of_header_index = d - compressed_text;
        d +=
        sprintf(d, "%s%d:",
            type, max_symbol_value
            );
        printf("# %s", compressed_text );
        for( int i=0; i<=max_symbol_value; i++ ){
            d +=
            sprintf(d, "%d",
                canonical_lengths[i]
                );
        };
        int end_of_header_index = d - compressed_text;
        d +=
        sprintf(d, ","); // end of netstring
        const int actual_header_size = end_of_header_index - start_of_header_index;
        printf("# actual header size: %d\n", actual_header_size);
        assert( actual_header_size == netstring_length );
        printf("# data ....\n");
        // FIXME: quick hack:
        // reserve 5 digits for length;
        // later overwrite with correct value.
        const int data_length_initial = 32767; // initial guess
        int data_length_index = d - compressed_text;
        d +=
        sprintf(d, "%d:", // start of netstring
            data_length_initial
            );

        d +=
        represent_items_with_codes(
            max_symbol_value,
            canonical_lengths,
            compressed_symbols, // 2 for binary, 3 for trinary, etc.
            bufsize, // const size_t bufsize,
            original_length,
            original_text,
            (d - compressed_text),
            compressed_text
            );
        const int actual_data_length = d - compressed_text;
        if(data_length_initial == actual_data_length){
            printf("# guessed correct length, nothing to do.\n");
        }else{
            // Quick hack:
            // reserve 5 digits for length;
            // later overwrite with correct value.
            // if it's 9999 or less,
            // left-pad with spaces.
            // FIXME: somehow get rid of that padding.
            printf("# %d: actual data length", actual_data_length);
            assert(actual_data_length <= 32767);
            sprintf(d+data_length_index, "% 5d:", // start of netstring
                actual_data_length
                );
        };
        assert(0);
        /*
        sprintf(d, "%d:%s%.*s,",
            netstring_length, type, original_length, original_text
            );
        */
        sprintf(d, ","); // end of netstring
        printf("# compressed.\n");
    };
    if(1){
        // currently only handles ASCII text.
        // FUTURE: handle arbitrary binary.
        // FUTURE: handle more than 257 source symbols.
        assert(0 == canonical_lengths[0]);
    };
    int huffman_data_size = original_length; // FIXME
    int huffman_header_size = 256; // FIXME
    // if it doesn't save any space to Huffman compress --
    // such as when all canonical lengths are the same --
    // fall back to simple pass-thru raw data.
    if( huffman_data_size + huffman_header_size >= original_length ){
        printf("# pass-through raw data.\n");
        char * d = compressed_text;
        char * type = "\n\n"; // pass-through type
        int netstring_length = original_length + strlen(type);
        sprintf(d, "%d:%s%.*s,",
            netstring_length, type, original_length, original_text
            );
    };
}

static int
get_compressed_block_length( const char * s ){
    // Later this will be restricted more,
    // but currently "%i"
    // supports base-16 if blocks begin with "0x" or "0X",
    // base-8 if it begins with 0,
    // and base 10 otherwise.
    int length = 0;
    sscanf( s, "%i", &length );
    assert( length <=  32768 );
    assert( length <= 0x8000 );
    assert( 0 <= length );
    return length;
}

/*
Given a block of compressed text
(starting with a compact representation
of the canonical list of lengths),
recover the uncompressed text.

For each block,
the decoder inspects the header to decide:
* pass-thru without changes
(the decoder simply deletes the header and footer of block)
(for maximum human-readability,
the header ends with a newline and the footer begins with a newline)
* read Huffman table header, and
decompress the following 8-bit binary Huffman-compressed data
(which may include NULL bytes)
* A quasi-human-readable representation
of compressed Huffman:
(to use as examples in
https://en.wikibooks.org/wiki/Data_Compression
)
** lines beginning with '#' are comments,
ignored by the decoder
** Text from '#' to the end of the line is a comment,
ignored by the decoder
** newlines and whitespace in the middle of data are ignored
** *any* of the standard base64
somehow (?) store the Huffman table,
then
** 6-bit base64url encoded Huffman data
** 4-bit hexadecimal encoded Huffman data
** "natural" representations of trinary and decimal Huffman
** ... other ? ...


Currently using
"netstring" format
for each block of data.
Currently
the "netstring length"
at the beginning of each block
is at most 2^15.
arbitrarily long streams of data
are handled as a sequence of blocks.
Each block
is between zero bytes of payload
(netstring "0:," total length: 3 bytes)
and (2^15) bytes of payload
(netstring "32768:..........,\n"
with 5 bytes of length + 3 bytes overhead
for the colon, comma, and newline,
total length: 2^15+8 bytes = 32776 bytes
).

FUTURE:
To reduce latency
and be friendlier to low-RAM embedded systems,
we may limit each block to a max of 4 KBytes (?).

Does it make any sense to
"leave out" certain lengths --
for example,
encode every byte length
from 0 to 256 bytes literally,
then have
"257" represent 1.5*256 bytes,
"258" represent 2*256 bytes,
"259" represent 3*256 bytes,
"260" represent 4*256 bytes,
"261" represent 5*256 bytes,
etc.
so that
certain amounts of bytes
are forced to be represented
as 2 blocks?

FUTURE:
Consider using
one of the other formats mentioned at
https://en.wikipedia.org/wiki/Comparison_of_data-serialization_formats
.
Perhaps "Smile" data format.
Perhaps (for human-readability) JSON format.

Currently the data inside each block
begins with 2 bytes of type indicator:
"\n\n": pass-through raw data
"\n#": metadata string (currently only used for debugging)
"\nX": Huffman table type 1 (human-readable)
"\nZ": Huffman-compressed data type 1 (human-readable)
(Each block of huffman table *should*
be immediately followed by Huffman-compressed data block.
).
(However,
it's fine to have a bunch of Huffman-compressed data blocks
without any Huffman table blocks between them --
each block typically uses the most-recently-defined Huffman table.
).

Currently,
the compressor
usually (?)
inserts a newline byte "\n" after the ","
at the end of the netstring block.
ANSI C defines sscanf() to use the same fomat as strtol(),
which (in this case correctly)
skips over that whitespace
when reading the decimal digits
at the start of the *next* block.
Future formats
may try to "save space"
by eliminating that ",\n" block footer.

FIXME:
Give a better user experience
if a user tries to "decompress"
a normal (uncompressed) file
or a "compressed" file
in some format other than the ones
this program recognizes.
I.e., compressed files
in GIF or GZIP or BZIP format,
or compressed files
generated by
incompatible far-past or far-future versions of this program.
Currently this program
uses a lot of assert()
for quicker debugging.


FIXME:
somehow decode
(perhaps in a special "header" block type?)
(perhaps a field in some/all block types?)
the semantic version number of this file.
So ideally this program
can tell whether it is a
"distant future" file with breaking changes,
a "near future / near past" file
that this program handles natively,
or
one or two major past versions
still correctly decoded by this program.

FUTURE:
Consider experimenting with other ways
of representing the Huffman table.
Currently each block of Huffman compressed data
starts with
DAV's Type 1 Huffman table.
Experiment with other options:
(a) different types of Huffman table
(perhaps, say, encode only the 10 most-frequent letters exactly,
and then assume the remaining letters are
practically equal-frequency --
hopefully the bits we lose using non-perfect Huffman lengths
are made up by requiring a much shorter header table).
(b) use less space in Huffman tables
by *remembering* the last N non-idendical Huffman tables
and then
occasionally starting a block
with something that tells the decoder
"This next block uses identically the same Huffman table
as the M'th non-identical Huffman table ago."
(c) Use less space in Huffman tables
by starting a block with something
that indicates
"This next block uses a Huffman table *similar*
to the M'th non-identical Huffman table ago,
but with these small changes".
Unclear exactly how to represent "small" changes
to a Huffman table.
One approach is
simply point-wise *subtract* the two lists of bitlengths,
and then
somehow encode the (presumably small) *difference* values;
Presumably the representation of those changes
can be made even smaller
by somehow taking advantage of the
Kraft inequality.


*/
static int
decompress(
    const int max_compressed_size,
    const char compressed_text[],
    const int max_decompressed_size,
    char decompressed_text[] // output
    ){

    const char * s = &compressed_text[0];
    char * d = &decompressed_text[0];

    // FIXME:
    // There seems to be a conflict between
    // (a) we want to use standard C string-handling functions,
    // the actual compressed data
    // and the actual decompressed data
    // *should*
    // be far less than the buffer size,
    // so it *should* be OK to slap a "\0"
    // as the last byte of both
    // so we can guarantee
    // standard C string-handling functions
    // which assume a "\0" byte,
    // we don't read
    // past the end of the buffer ...
    // (b) but we *also* want to handle
    // compressed text stored in ROM.
    const int length = get_compressed_block_length( s );
    // In a netstring, data is the *next* character
    // after the first ":"
    const char * colon_pos = strchr( s, ':' );
    assert(colon_pos); // there *should* be a ":" character.
    const int end_of_block_index = colon_pos - s + length + 1;
    assert( end_of_block_index < max_compressed_size );
    // netstrings *should* end with ',' after end of data.
    assert( ',' == s[end_of_block_index] );
    assert( '\n' == s[end_of_block_index + 1] );

    // block type indicator
    // in the next 2 bytes after the colon:
    assert( '\n' == colon_pos[1] );
    char block_type = colon_pos[2];
    const char * data_start = colon_pos + 3;
    /*
"case blocks in switch statements should have curly braces."
--
https://codingart.readthedocs.io/en/latest/c/Formatting.html

"C Switch-case curly braces after every case"
https://stackoverflow.com/questions/4241545/c-switch-case-curly-braces-after-every-case

    */
    switch( block_type ){
    default: {
        // unknown block type
        assert(0);
        }; break;
    case '\n': { // pass-through raw data
        printf("# raw data:\n");
        assert( max_decompressed_size > length );
        memcpy( d, data_start, length );
        d[length+1] = '\0';
        }; break;
    case '#': { // metadata string
        // perhaps we should just skip?
        printf("# skipping metadata.\n");
        }; break;
    case 'X': { // human-readable Huffman table type 1
        // FIXME:
        assert(0);
        }; break;
    case 'Z': { // human-readable Huffman data type 1
        assert( max_decompressed_size >= 4000  );
        // FIXME:
        assert(0);
        }; break;

        }; // end switch().

    return length;
}

/*
"Practical Huffman coding: Maximum length of Huffman symbol"
by Michael Schindler
http://www.compressconsult.com/huffman/#maxlength
mentions two independent worst-case Huffman lengths:
<q>
No huffman code can be longer than alphabetsize-1. Proof: it is impossible to construct a binary tree with N nodes and more than N-1 levels.
</q>
But the worst-case Huffman length may be much shorter than that
if you limit the number of symbols in a block:
<q>
the sequence is as follows (the samples include the fake samples to give each symbol a nonzero probability!):

    #samples 	maximum codelength
    1....2 	1
    3....4 	2
    5....7 	3
    8...12 	4
    13...20 	5
    #samples 	maximum codelength
    21...33 	6
    34...54 	7
    55...88 	8
    89..143 	9
    144..232 	10

the width of each range is the lower end of the previous range, so the next would be: 233 to 233+144-1=376 samples give a maximum codelength of 11.

An example for a tree with depth 6 and 21 samples (count for each symbol given) would be ((((((1,1),1),2),3),5),8). (oh no - Fibonacci numbers again :) 
</q>


https://stackoverflow.com/questions/57036603/in-zlib-what-happen-when-the-huffman-code-lengths-for-the-alphabets-exceed-maxim
mentions that
<q>
zlib normally limits a deflate block to 16384 symbols. For that number, the maximum Huffman code length is 19. That comes from a Fibonacci-like (Lucas) sequence of probabilities, not your powers of two. (Left as an exercise for the reader.)
</q>


"Maximum size of Huffman codes for an alphabet containing 256 letters"
https://cs.stackexchange.com/questions/75542/maximum-size-of-huffman-codes-for-an-alphabet-containing-256-letters
mentions
<q>

In theory, 256 characters can have probabilities

    2^−1, 2^−2, ..., 2^−255, 2^−255

(yes, the last one is 2^−255, not 2^−256
), so there could be two codes of 255 bits.

In practice,
if you compress let's say a document of 1 Gigabyte,
no character can have a probability less than 2^−30,
so the maximum length would be 30 bits.

And again in practice,
you can
make a decoder faster if it has a limited maximum number of bits, say 14.
That's called "length limited Huffman codes".
There's a rather complicated algorithm
that can be given the maximum length l of any code that you want to achieve
(obviously ≥ 8 for 256 codes),
and it will calculate the optimal codes under that restriction.
Obviously not optimal globally,
but usually very close,
if you limit the length to 14 bits, for example. 

</q>


DAV:
However it is unclear if these are using a "good algorithm"
(that, whenever we have the option -- there's a tie,
we choose to merge leaves or shallow branches rather than deep branches)
or a "perverse algorithm"
(that, whenever we have the option -- there's a tie,
we choose to merge the deepest branch rather than leaves or shallow branches
).
Both the "good" and the "perverse" algorithms compress the data
into the same total number of bits ...
(a proof of this is requested at
"Average codeword length is the same in any Huffman encoding schemes"
https://math.stackexchange.com/questions/2661204/average-codeword-length-is-the-same-in-any-huffman-encoding-schemes
).

"Two Huffman trees for one corpus. How is it possible?"
https://math.stackexchange.com/questions/705521/two-huffman-trees-for-one-corpus-how-is-it-possible
also points out that
that not only do we get different Huffman codes
by randomly swapping internal nodes left/right (1/0),
but we can get different *shape* Huffman trees.


DAV:
In either case, the max-depth tree looks the same:

             /\
            /\ L
           /\ L
       ....  L
      /\
     /\ L
    /\ L
    L L

The perverse algorithm allows:

           ....
            24
           / \
          15  \
         / \   L count=9
        9   \
       / \   L count=6
       6  \
      / \  L count=3 (tied for the (1(1:1)) )
      3  \
     / \  L count=2 (tied for the 1:1)
     2  \
    / \  L count=1
    L L 
    1 1

The "good" algorithm allows:

           ....
            28
           / \
          17  \
         / \   L count=11
        10  \
       / \   L count=7
       6  \
      / \  L count=4
      3  \
     / \  L count=3 (1 more than the (1(1:1)) )
     2  \
    / \  L count=1
    L L 
    1 1







Other things related to maximum Huffman length:


https://math.stackexchange.com/questions/1635832/word-lengths-of-optimal-binary-code

https://math.stackexchange.com/questions/2661166/maximimze-a-huffman-code-length

"How many bits does the longest encoded symbol have, using an Huffman coding on a given probability distribution?"
https://math.stackexchange.com/questions/835451/how-many-bits-does-the-longest-encoded-symbol-have-using-an-huffman-coding-on-a


"On the Maximum Length of Huffman Codes"
by Michael Buro
https://skatgame.net/mburo/ps/huff.pdf

*/

/*
Other things related to (non-binary) n-ary Huffman codes:

https://math.stackexchange.com/questions/653112/encoding-a-channel-with-huffman-code
discusses trinary Huffman,
and the necessity of adding dummy symbols.



[data compression: word-oriented Huffman]

"Huffman-coding English words"
by Project Nayuki
https://www.nayuki.io/page/huffman-coding-english-words
DAV: has some fun experiments;
for human-readability, it compressed each English *word*
(word-oriented Huffman)
into a "codeword" made only of letters
using 52-ary Huffman coding (!)
which it points out is "wildly inefficient".
symbols are passed through unchanged.
Also,
it points out that
"Some words appear in the text only once (hapax legomena)."
and rather than include them in the english_word:codeword table
at the beginning of the compressed file,
it spells them out one letter at a time.
The individual *letters* are often encoded as multi-letter Huffman-compressed *words*,
because the frequency used is (correctly)
counted as only the times the letter is used "by itself",
and not inside a (non-hapax-legomena) compressed word.
DAV wonders if perhaps we could make this
even more human-readable
by noticing the surprisingly often case
where a 2-letter English "word" (such as "it")
is encoded into a 2-letter Huffman-compressed codeword,
and swapping things around
so that that 2-letter English word
is represented by itself in the compressed file.
DAV found this Project Nayuki Huffman page from a link on
"What Is Huffman Coding? (baseclass.io)"
https://news.ycombinator.com/item?id=26203282
DAV:
It's tempting to "save" all the space
in the initial dictionary of English words to Huffman codes
by reserving some special symbol
and spelling out the word the first time it is used --
but it seems likely that the amount of space it takes
to spell out that word
the first time it is used
is just as much as
spelling out that word
in the initial dictionary,
so no net improvement in compression.
(It *does* improve latency ...)


DAV:
perhaps it would be fun to do something similar
for short text data compression --
perhaps format-preserving compressing fields in CSV files,
where we are careful to maintain the structure of the CSV files
by always preserving the commas and newlines unchanged
and avoiding
commas, newlines, or double-quotes
in the compressed fields ...
...
If we allow, say,
base64URL character set in the fields,
the simplest would seem to be
(as Nayuki does)
a 64-ary Huffman compression,
but
(as Nayuki points out)
that is likely "wildly inefficient".
Perhaps it would be more efficient to
use (octal) 8-ary Huffman compression
and pack 2 octal digits per base64URL character
(and somehow "round up"
each field that packs into an odd number
of octal digits
)
or use 4-ary or 2-ary or binary Huffman compression,
even though that "throws away"
some padding bits at the end of each field.
Or perhaps somehow
"re-use" some of those padding bits?
Say, we use (octal) 8-ary Huffman compression.
And somehow (unclear if this is reasonable)
every codeword uses at least 2 octal digits.
Then when decoding any one *row* of the CSV file,
with several compressed 
fields,
if we hit the comma at the end of the field
before we hit the end of a codeword,
we preserve those digits
and use them at the beginning of the *next* compressed field.
(However, when we hit the newline
at the end of the record,
then we flush those padding digits
and start fresh on the next row.
).
It's unclear
how to stick in the Huffman header "dictionary"
while still preserving the CSV format.

DAV:
perhaps it would be fun to do something similar
for short text data compression --
perhaps format-preserving compressing fields in
text, perhaps even the same Alice in Wonderland benchmark.
If the compressed payload *always* alternates between
* base64 digits representing compressed *words*
* non-base-64 spaces, punctuation, etc. passed through unchanged
then (as Nayuki points out)
we don't really *need* the prefix property,
so perhaps we can use that to get slightly more compression.
(Perhaps one special "escape" word,
followed by a space,
to indicate
"a literal word follows"
for hapax legomena
that's not in the dictionary.
).


*/





int
test_various_table_representations(
        int max_symbol_value,
        int symbol_frequency[max_symbol_value],
        int canonical_length[max_symbol_value]
){

    int uncompressed_length=0;
    int standard_huffman_length=0;
    int longest_symbol=0;
    int shortest_nonzero_symbol=INT_MAX;
    for(int i=0; i<(max_symbol_value+1); i++){
        int lb = canonical_length[i];
        uncompressed_length += symbol_frequency[i]*8;
        standard_huffman_length += symbol_frequency[i]*lb;
        longest_symbol = imax(longest_symbol, lb);
        if(lb){
            shortest_nonzero_symbol = imin(shortest_nonzero_symbol, lb);
        };
    };
    printf("# %i bits: longest symbol\n", longest_symbol );
    printf("# %i bits: uncompressed length\n", uncompressed_length );
    if( longest_symbol < 16 ){
        printf("# %i = %i + %i: table of 256 nybbles + huffman length\n",
            256*4 + standard_huffman_length,
            256*4, standard_huffman_length
        );
        // FIXME: length-limited huffman
        /*
        printf("# %i = %i + %i bits: only 16 most-common symbols in table",
            ??? + limited_length_huffman
            ???, limited_length_huffman
        );
        */
        // FIXME: only 16 most-common symbols
        // The table contains 12 bits per symbol,
        // the literal value of the symbol + 4 bits of length,
        // perhaps packed as <symbol><symbol><length length>.
        /*
        printf("# %i = %i + %i bits: only 16 most-common symbols in table",
            16*12 + common_length_huffman
            16*12, common_length_huffman
        );
        */
    }else{
        printf("################### unexpectedly long symbol !!!!!");
    };
    return 0;
}

/*
inspired by
zbyszek
https://stackoverflow.com/questions/994593/how-to-do-an-integer-log2-in-c/22418446#22418446
*/
static inline int
log2i(int x) {
    assert(x > 0);
    // "...clz()" is for "int"
    // "...clzl()" is for "long int".
    return sizeof(int) * 8 - __builtin_clz(x) - 1;
}

/*
inspired by
https://stackoverflow.com/questions/3272424/compute-fast-log-base-2-ceiling
*/
int
ceil_log2(int x){
    if(2 >= x){ return (1); };
    return ( log2i( x - 1 ) + 1 );
}

int
find_compressed_data_size(
    int max_symbol_value,
    int symbol_frequencies[max_symbol_value+1],
    int canonical_lengths[max_symbol_value+1],
    int compressed_symbols
    ){
    assert(compressed_symbols);
    int max_length = 0;
    int min_length = INT_MAX;
    int nonzero_symbols = 0;
    int data_size = 0;
    int uncompressed_length = 0;
    for( int i=0; i<(max_symbol_value+1); i++){
        int a_length = canonical_lengths[i];
        max_length = imax( max_length, a_length );
        if(0 < a_length){
            min_length = imin( min_length, a_length );
            nonzero_symbols++;
            data_size += a_length * symbol_frequencies[i];
            uncompressed_length += symbol_frequencies[i];
            assert( 0 < symbol_frequencies[i] );
        }else{
            assert( 0 == a_length );
            assert( 0 == symbol_frequencies[i] );
        };
    };
    printf("# %i is the max length!!!!!!!!!!!!!!!!!!!!!\n", max_length);
    printf("# %i is the min length.\n", min_length);
    printf("# nonzero_symbols: %i.\n", nonzero_symbols );
    int uniform_bits = ceil_log2(nonzero_symbols);
    printf("# uniform_bits: %i\n", uniform_bits);
    printf("# uniform data size: %i\n",
        uniform_bits * uncompressed_length
        );
    printf("# compressed data_size, not including header: %i\n",
        data_size
        );

    return data_size;
}

void
next_block(void){
    printf("# Starting next block...\n");
    // FIXME: use a larger buffer,
    // perhaps with calloc() or realloc() or both?
    const int bufsize = 65000; // FIXME: const size_t bufsize = 65000;
    char original_text[bufsize+1];
    size_t used = load_more_text( stdin, bufsize, original_text );
    // FUTURE: support arbitrary data, including '\0' character.
    size_t original_length = strlen( original_text );
    // FIXME: doesn't yet support reading '\0' bytes
    assert( original_length == used );



    // FIXME: support arbitrary number of symbols.
    const int max_symbol_value = 258;
    int symbol_frequencies[max_symbol_value+1];
    printf("# finding histogram.\n");
    histogram( original_text, max_symbol_value, symbol_frequencies );
    assert( 0 == symbol_frequencies[258] );
    int compressed_symbols = 3; // 2 for binary, 3 for trinary, etc.
    // FUTURE: length-limited Huffman?

    /*
    int canonical_lengths[text_symbols];
    */
    printf("# finding canonical lengths.\n");
    int canonical_lengths[max_symbol_value+1];
    for( int i=0; i<(max_symbol_value+1); i++){
        canonical_lengths[i] = 0;
    };
    // int canonical_lengths[nonzero_text_symbols] = {};
    huffman(
        max_symbol_value, symbol_frequencies,
        compressed_symbols,
        canonical_lengths
    );
    printf("# now we have the canonical lengths ...\n");
    debug_print_table( max_symbol_value, canonical_lengths, compressed_symbols );
    test_various_table_representations(
        max_symbol_value,
        symbol_frequencies,
        canonical_lengths
    );
    printf("# compressing text.");
    char compressed_text[bufsize+1];
    compress(
        max_symbol_value,
        canonical_lengths,
        compressed_symbols,
        bufsize,
        original_length,
        original_text,
        compressed_text
        );
    printf("# decompressing text.");
    char decompressed_text[bufsize+1];
    size_t decompressed_length =
    decompress( bufsize+1, compressed_text, bufsize+1, decompressed_text );
    // FUTURE: fix so it correctly handles text with '\0' bytes.
    size_t text_length = strlen( decompressed_text );
    assert( text_length <= 0x8000 );
    assert( original_length == decompressed_length );
    assert( original_length == text_length );
    if( memcmp( original_text, decompressed_text, original_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", original_text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };
}

void test_setup_nodes(){
    printf("# starting test_setup_nodes():\n");
#define    max_leaf_value_doubled (600)
    const int list_length = max_leaf_value_doubled;
    struct node list[max_leaf_value_doubled];
    // int sorted_index[max_leaf_value_doubled];

#define max_symbol_value (300)
    int symbol_frequencies[max_symbol_value+1] = {0};
    symbol_frequencies['a'] = 10;
    symbol_frequencies['c'] = 9;

    setup_nodes(
        list_length, list,
        // sorted_index,
        max_symbol_value,
        symbol_frequencies
    );
    printf("# Done test_setup_nodes():\n");
#undef max_leaf_value_doubled
#undef max_symbol_value
}

void
test_next_block(void){
    printf("# Starting next block...\n");
    // FIXME: use a larger buffer,
    // perhaps with calloc() or realloc() or both?
    const size_t bufsize = 65000;
    char original_text[65000+1] =
        ""
"/* n_ary_huffman.c"
"WARNING: version  2021.00.01-alpha : extremely rough draft."
"2021-10-25: started by David Cary"
""
"A few experiments with"
"n-ary Huffman compression."
""
"Many Huffman data compression algorithms"
"2 output symbols (binary)"
"and"
"around 257 input symbols."
"(Often"
"256 symbols: all possible bytes"
"DEFLATE has 288 symbols in its main Huffman tree:"
"0..255: all possible literal bytes 0-255"
"256: end-of-block symbol"
"257-285: match lengths"
"286, 287: not used, reserved and illegal."
"DEFLATE has 32 symbols in its 'distance' tree."
")Z"
"*/"
        "";
    size_t original_length = strlen( original_text );


/*
           0 1 2 3 4 5 6 7 8 9
length 1:  e t a n s l       space
length 2:  - . 0 1 5 7 8 : A D
length 2:  E H b c d f g h i m
length 2:  o p r u v y
length 3:  ' ( ) * , / 3 6 C F
length 3:  G I L M N O R T W Z
length 3:  _ k w x


           0 1 2 3 4 5 6 7 8
length 1:  e   a n s       space
length 2:  - . 0 1 2 5 6 7 8
length 2:  : A D E H _ b c d
length 2:  f g h i l m o p r
length 2:  t u v w x y
length 3:  ' ( ) * , / 3 C F
length 3:  G I L M N O R T W
length 3:  Z k
*/

    // FIXME: support arbitrary number of symbols.
    const int max_symbol_value = 258;
    int symbol_frequencies[max_symbol_value+1];
    symbol_frequencies[258] = 0xBEEF; // for debugging: canary sentinel to test zeroing.
    printf("# finding histogram.\n");
    histogram( original_text, max_symbol_value, symbol_frequencies );
    assert( 0 == symbol_frequencies[258] );
    int compressed_symbols = 3; // 2 for binary, 3 for trinary, etc.
    assert(compressed_symbols);
    // FUTURE: length-limited Huffman?

    printf("# finding canonical lengths.\n");
    int canonical_lengths[max_symbol_value+1];
    for( int i=0; i<(max_symbol_value+1); i++){
        canonical_lengths[i] = 0;
    };
    assert(0 == canonical_lengths[0]);
    // int canonical_lengths[nonzero_text_symbols] = {};
    huffman(
        max_symbol_value, symbol_frequencies,
        compressed_symbols,
        canonical_lengths
    );
    printf("# now we have the canonical lengths ...\n");
    debug_print_table( max_symbol_value, canonical_lengths, compressed_symbols );
    int compressed_data_size = 
    find_compressed_data_size(
        max_symbol_value,
        symbol_frequencies,
        canonical_lengths,
        compressed_symbols
        );
    printf("# compressed_data_size, not including header: %i symbols\n",
        compressed_data_size
        );
    test_various_table_representations(
        max_symbol_value,
        symbol_frequencies,
        canonical_lengths
    );
    printf("# compressing text...\n");
    char compressed_text[bufsize+1];
    compress(
        max_symbol_value, canonical_lengths, compressed_symbols,
        bufsize,
        original_length,
        original_text,
        compressed_text
    );
    printf("# decompressing text.\n");
    char decompressed_text[bufsize+1];
    decompress( bufsize, compressed_text, bufsize, decompressed_text );
    size_t decompressed_length = strlen( decompressed_text );
    assert( original_length == decompressed_length );
    if( memcmp( original_text, decompressed_text, original_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", original_text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    }
}


void
short_test_next_block(void){
    printf("# short_test_next_block ...\n");
    // FIXME: use a larger buffer,
    // perhaps with calloc() or realloc() or both?
    const size_t bufsize = 65000;
    char original_text[65000+1] =
        ""
"/* n_ary_huffman.c"
"2021-10-25: started by David Cary"
        "";
    size_t original_length = strlen( original_text );

    // FIXME: support arbitrary number of symbols.
    const int max_symbol_value = 258;
    int symbol_frequencies[max_symbol_value+1];
    symbol_frequencies[258] = 0xBEEF; // for debugging: canary sentinel to test zeroing.
    printf("# finding histogram.\n");
    histogram( original_text, max_symbol_value, symbol_frequencies );
    assert( 0 == symbol_frequencies[258] );
    int compressed_symbols = 3; // 2 for binary, 3 for trinary, etc.
    assert(compressed_symbols);
    // FUTURE: length-limited Huffman?

    printf("# finding canonical lengths.\n");
    int canonical_lengths[max_symbol_value+1];
    for( int i=0; i<(max_symbol_value+1); i++){
        canonical_lengths[i] = 0;
    };
    assert(0 == canonical_lengths[0]);
    // int canonical_lengths[nonzero_text_symbols] = {};
    huffman(
        max_symbol_value, symbol_frequencies,
        compressed_symbols,
        canonical_lengths
    );
    printf("# now we have the canonical lengths ...\n");
    debug_print_table( max_symbol_value, canonical_lengths, compressed_symbols );
    assert(0);
    int compressed_data_size = 
    find_compressed_data_size(
        max_symbol_value,
        symbol_frequencies,
        canonical_lengths,
        compressed_symbols
        );
    printf("# compressed_data_size, not including header: %i symbols\n",
        compressed_data_size
        );
    test_various_table_representations(
        max_symbol_value,
        symbol_frequencies,
        canonical_lengths
    );
    printf("# compressing text...\n");
    char compressed_text[bufsize+1];
    compress(
        max_symbol_value, canonical_lengths, compressed_symbols,
        bufsize,
        original_length,
        original_text,
        compressed_text
    );
    printf("# decompressing text.\n");
    char decompressed_text[bufsize+1];
    decompress( bufsize, compressed_text, bufsize, decompressed_text );
    size_t decompressed_length = strlen( decompressed_text );
    assert( original_length == decompressed_length );
    if( memcmp( original_text, decompressed_text, original_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", original_text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    }
}

// return 1 (True) if the arrays are equal.
bool
arrays_equal(
    int array_length,
    int a[array_length],
    int b[array_length]
){
    for(int i=0; i<array_length; i++){
        bool same = (a[i] == b[i]);
        if(!same){
            printf(
                "First difference: a[%i]=%i ; b[%i]=%i.\n", 
                i, a[i], i, b[i]
                );
            return false;
        };
    };
    return true;
}

void
test_convert_lengths_to_encode_table(void){
    int max_symbol_value = 20;
    int encode_length_table[80] = {0};
    unsigned int encode_value_table[80] = {0};
    if(1){
        /*
        int length_table_length = NUM_ELEM( length_table );
        */
        int length_table[80] = {0, 0, 1, 1, 1};
        convert_lengths_to_encode_table(
            max_symbol_value,
            length_table,
            3, // trinary output
            encode_length_table,
            encode_value_table
            );
        assert( arrays_equal( (max_symbol_value+1), length_table, encode_length_table ) );
        int expected_length_table[80] = {0, 0, 1, 1, 1};
        int expected_value_table[80] = {0, 0, 0, 1, 2}; // trinary
        assert( arrays_equal( 80, expected_value_table, (int *)encode_value_table ) );
        assert( arrays_equal( 80, expected_length_table, encode_length_table ) );
    };
    if(1){
        // test with 8 equal-frequency items,
        // which (in trinary) should lead to all items of length 2
        // (and 1 unused "dummy" codeword).
        int length_table[80] = {0,0,
            2, 2, 2, 2,
            2, 2, 2, 2,
        };
        convert_lengths_to_encode_table(
            max_symbol_value,
            length_table,
            3, // trinary output
            encode_length_table,
            encode_value_table
            );
        assert( arrays_equal( (max_symbol_value+1), length_table, encode_length_table ) );
        unsigned int expected_value_table[80] = {0,0,
            0, 1, 2, 3,
            4, 5, 6, 7,
        };
        assert( arrays_equal( 80, (int *)expected_value_table, (int *)encode_value_table ) );
    };
    if(1){
        // test with 9 equal-frequency items,
        // which (in trinary) should lead to all items of length 2
        // (and zero unused "dummy" codewords).
        int length_table[80] = {0,0,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2,
        };
        convert_lengths_to_encode_table(
            max_symbol_value,
            length_table,
            3, // trinary output
            encode_length_table,
            encode_value_table
            );
        assert( arrays_equal( (max_symbol_value+1), length_table, encode_length_table ) );
        unsigned int expected_value_table[80] = {0,0,
            0, 1, 2, 3,
            4, 5, 6, 7,
            8
        };
        assert( arrays_equal( 80, (int *)expected_value_table, (int *)encode_value_table ) );
    };

}

void run_tests(void){
    short_test_next_block();
    test_convert_lengths_to_encode_table();
    test_summarize_tree_with_lengths();
    test_setup_nodes();
    test_next_block();
    next_block();
}

int main(void){
    run_tests();
    // next_block();
    return 0;
}

/* vim: set shiftwidth=4 expandtab ignorecase smartcase incsearch softtabstop=4 background=dark : */

