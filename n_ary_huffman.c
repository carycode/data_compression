
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
base9 digits
or
base27 digits
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

FUTURE: length-limited Huffman?

FUTURE: use size_t rather than int for array lengths?

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
consider using
https://en.wikipedia.org/wiki/C_data_types#Fixed-width_integer_types
*/

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
    char leaf_value;
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

void
generate_huffman_tree(
    // const int text_symbols,
    const int list_length,
    struct node list[list_length], // in-out: updated
    const int compressed_symbols,
    const int max_leaf_value
){
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
    printf("# using %i dummy nodes.\n", dummy_nodes);
    printf("# max_leaf_value: %i\n", max_leaf_value);
    for(int i=(max_leaf_value+1); i<(max_leaf_value + 1 + dummy_nodes); i++){
        sorted_index[i] = i;
        list[i].count = 1; // minimum count for dummy nodes.
    };
    assert(1 == list[259].count ); // dummy node
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
        int leaf_value = list[i].leaf_value;
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
        { true, 9, 0, 0, 'a', 2 },
        { true, 9, 0, 0, 'b', 2 },
        { false, 4, 0, 1, 0, 0 }
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
        { true, 9, 0, 0, 'a', 4 },
        { true, 9, 0, 0, 'b', 3 },
        { true, 8, 0, 0, 'c', 3 },
        { false, 17, 1, 2, 0, 4 },
        { false, 26, 0, 3, 0, 0 }
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

/*
Given a list of lengths
(one length for each symbol)
and a block of uncompressed text,
generate a block of compressed text
(starting with a compact representation
of the canonical list of lengths).
*/
void compress(){
    // FIXME:
    assert(0);
}

/*
Given a block of compressed text
(starting with a compact representation
of the canonical list of lengths),
recover the uncompressed text.
*/
void decompress(){
    // FIXME:
    assert(0);
}

// apparently not yet in standard libraries
static int
imax( int a, int b ){
    return ((a<b)? b : a);
}
static int
imin( int a, int b){
    return ((a<b)? a : b);
}

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
        uncompressed_length = symbol_frequency[i]*8;
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

void
next_block(void){
    printf("# Starting next block...\n");
    // FIXME: use a larger buffer,
    // perhaps with malloc() or realloc() or both?
    const size_t bufsize = 65000;
    char original_text[bufsize+1];
    size_t used = load_more_text( stdin, bufsize, original_text );
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
    compress( max_symbol_value, canonical_lengths, compressed_symbols, compressed_text );
    printf("# decompressing text.");
    char decompressed_text[bufsize+1];
    decompress( compressed_text, decompressed_text );
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
    // perhaps with malloc() or realloc() or both?
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
    /*
    size_t original_length = strlen( original_text );
    */

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
    compress( max_symbol_value, canonical_lengths, compressed_symbols, compressed_text );
    /*
    printf("# decompressing text.");
    char decompressed_text[bufsize+1];
    decompress( compressed_text, decompressed_text );
    size_t decompressed_length = strlen( decompressed_text );
    assert( original_length == decompressed_length );
    if( memcmp( original_text, decompressed_text, original_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", original_text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    }
    */
}


void run_tests(void){
    test_next_block();

    /*
    test_summarize_tree_with_lengths();
    test_setup_nodes();
    next_block();
    */
}

int main(void){
    run_tests();
    // next_block();
    return 0;
}

/* vim: set shiftwidth=4 expandtab ignorecase smartcase incsearch softtabstop=4 background=dark : */

