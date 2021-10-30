
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

int main(void){
    char * original_text = load_text();
    size_t original_length = strlen( original_text );
    int plaintext_symbols = 258;
    int symbol_frequencies * = histogram( text, plaintext_symbols );
    int compressed_symbols = 3; // 2 for binary, 3 for trinary, etc.
    // FUTURE: length-limited Huffman?
    int * canonical_lengths = huffman( byte_frequencies, plaintext_symbols, compressed_symbols);
    debug_print_table( canonical_lengths , plaintext_symbols, compressed_symbols );
    char * compressed_text = compress( canonical_lengths, plaintex_symbols, compressed_symbols);
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

