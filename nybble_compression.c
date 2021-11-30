/* nybble_compression.c
WARNING: version 2015.00.00.03-alpha : extremely rough draft.
2021-10-13: forked from small_compression.c
2015-05-24: David Cary started.

a data compression algorithm for small embedded systems.

2021-09-15: DAV: I came up with this
simple nybble-oriented data compression:

* read a nybble at a time
(first the hi nybble, then the lo nybble)
* output a byte at a time
* If the hi bit of the nybble is "0",
then output the rest of this nybble
and the following nybble literally
as a byte (i.e.,
bytes 0x00..0x7F,
7-bit ASCII text,
represents itself)
* if the hi bit of the nybble is "1",
then use the remaining 3 bits of this nybble
to lookup one of 8 bytes to emit --
ideally the 8 most-common bytes in this context.

When we expect ASCII text,
perhaps initialize those 8 bytes
to the most-common letters --
space, e, t, a, ... etc.
space, e, t, a,  o, i, n, s.

Rather than literally counting
the most-frequent bytes,
perhaps simply
move-to-front
whatever actual byte
occurs in this context.
(If the actual byte
is not already in that list of 8 bytes,
drop the oldest byte from the list).

With null context,
this requires
a single array of
8 bytes
to remember those most-frequent letters.

With a full byte of context,
this requires
256 * 8 bytes
to remember those most-frequent letters --
perhaps too much to really be "simple".

Perhaps something in-between
would be appropriate
for Arduino?
Perhaps one of:
(a)
After emitting a byte,
drop the hi bit
(almost always 0 for ASCII text
and so not very useful)
and use the next-highest 4 bits
as 4 bits of context.
This requires
2^4 * 8 = 16 * 8 = 128 bytes of context RAM.
(b)
A few categories,
perhaps
* whitespace
* vowel
* consonant
* digit
* symbol
These 5 categories
require
5 * 8 = 40 bytes of context RAM.

How to handle full 8-bit data?
(some sort of escape?)
Should we try to avoid
creating a 0x00 byte in the compressed text?
If not, how else are we going to mark end-of-text?

Avoiding the 0x00 byte in compressed text
when there are no 0x00 bytes in the plaintext
seems pretty easy:
3 cases:
0x00 compressed byte when this byte represents a literal:
cannot happen if the plaintext has no 0x00 bytes.
0x00 byte when the hi nybble of this byte represents
a compressed byte:
cannot happen because we define
all compressed bytes
have a MSbit=1.
0x00 byte when the hi nybble of this byte represent
the second nybble of a literal byte:
to avoid this problem,
perhaps flip the meaning of the hi bit of the lo nybble
in this case,
so that
the compressor has the option,
after
detecting that the hi nybble is 0x0,
to force
the hi bit of the low nybble to be "1"
(avoiding the 0x00 byte)
and having the next 7 bits represent
any arbitrary full 7-bit literal.

Trying to keep literals aligned to bytes:
* every byte with '0' as the MSB is a literal
* when decoding a byte with '1' as the MSB,
the hi nybble is a compressed nybble.
there are two cases:
** Simple case: both nybbles of that byte are compressed nybbles.
** awkward case: the lo nybble is the first half of a literal.
The "easy" thing to do is to continue reading
the next nybble ...
but it's possible that we can have a long string of literals
that is off by one nybble.

To try to keep literal bytes
aligned on byte boundaries,
we can
look ahead to the next byte B with MSB = '1'
(temporarily skipping over literal bytes with MSB=0)
and use up the least-significant nybble of that compressed byte
with the lo nybble of the current byte
to synthesize a single unaligned literal byte;
*then* emit the literal bytes that we skipped over,
*then* decompress the most-significant nybble of that byte B,
then continue normally
with the byte after B.
This occasionally has 1 literal unaligned,
but never a long string of unaligned literals.

Is there a better way
to keep literal bytes
aligned on byte boundaries?

Perhaps something more like:
* every literal is a byte that represents itself
* long strings of literals
end with a byte B with MSB = '1'.
* The "nice" case is when
that byte is completely used up
decoding 2 (or more?) compressed bytes.
In more generality,
when we have an even number of compressed bytes
between uncompressed literal bytes.
* The awkward case is when
we have an odd number of compressed bytes
between literal bytes.
That seems like it would be easy to fix:
we have two modes:
exactly 1 leftover nybble,
or zero leftover nybbles.
When we have zero leftover nybbles,
when we hit a byte with MSbit=1,
we decompress the hi nybble.
If the lo nybble also has MSbit=1,
we decompress it and we end up with zero leftover nybbles,
as before.
If the lo nybble has MSBit=0,
then we store that nybble in a buffer for later use.
This seems to result
in an unbounded number of buffered nybbles
when we alternate
literal - nybble - literal - nybble - literal ...
Any other case of an odd number of non-literals
between literals is easy to fix:
if we have zero leftover nybbles,
store a nybble in a buffer for later use.
If we have 1 leftover nybbles,
use it up in this string of compressed nybbles
(literal - byte - literal
can decompress the byte + leftover nybble into 3 characters).
i.e., if we need to emit 3 non-literals between literals,
if we have zero leftover nybbles
compress those 3 into 2 bytes
(with a leftover nybble);
if we have a leftover nybble,
compress those 3 into 1 byte
(and put the third into the leftover nybble).


Worst-case expansion of 7-bit data is 1:1 (no expansion).



The previous byte of context
indicates which list of common bytes (in this context) to look up the next byte in.

Avoid using the 0x00 byte in the compressed text.

FUTURE:
compare compression effectiveness to
* generalized 3-bit prediction
* generalized 2-bit prediction
* the 1-bit RFC1968 PPP Predictor Compression Protocol
( https://en.wikibooks.org/wiki/Data_Compression/Markov_models#PPP_Predictor_Compression_Protocol )

DAV's 2-bit prediction (8 or 9 bits per literal):
(This assumes that the decompressed string *never* has any hi-bit-set bytes or 0x00 bytes).
* If the bit-buffer is empty, and the next byte is a 0x00 byte, that's the end of this block. Done!
* If the bit-buffer is empty, and the next byte starts with 0, emit that byte as a literal byte.
(and insert that byte into the predictor table for this context).
(and leave the bit-buffer empty).
* If the bit-buffer is empty, and the next byte starts with 1, read that byte into the bit-buffer.
(After we do this, the next bit we pull from the flag buffer is that '1' bit).
* If the bit-buffer is not empty, pull the next bit from the bit-buffer and decide what to do next:
** 0: literal: pull the next full byte from the compressed stream and emit it literally on the decompressed output stream.
** 1: compressed: pull the next bit from the bit-buffer (if the bit-buffer is empty, pretend like we pulled a '0' bit).
*** 0: emit most-common byte for this context; 1: emit second-most-common byte for this context.

DAV's 2-bit prediction (8 or 10 bits per literal)
(This allows all possible bytes in string, including hi-bit-set bytes and 0x00 bytes).
* If the bit-buffer is empty, and the next byte is a 0x00 byte, that's the end of this block. Done!
* If the bit-buffer is empty, and the next byte starts with 0, emit that byte as a literal byte
(and insert that byte into the predictor table for this context).
(and leave the bit-buffer empty).
* If the bit-buffer is empty, and the next byte starts with 1, read that byte into the bit-buffer.
(After we do this, the next bit we pull from the flag buffer is that '1' bit).
* If the bit-buffer is not empty, pull the next 2 bits from the bit-buffer and decide what to do next:
** 11: literal: pull the next full byte from the compressed stream and emit it literally on the decompressed output stream.
(This is used to emit hi-bit-set bytes, and to emit 0x00 bytes, as well as bytes that are not common in this context).
** 10: emit most-common byte for this context
** 01: emit second-most-common byte for this context.
** 00: emit third-most-common byte for this context.

DAV's nybble prediction:
similar to the 1-bit RFC1968 PPP Predictor Compression Protocol,
except uses nybbles as symbols:
* If the bit-buffer is empty, and the next byte is a 0x00 byte, that's the end of this block. Done!
* If the bit-buffer is empty, and the next byte starts with 0, emit that byte as a literal byte
(and insert those 2 nybbles into the predictor table for this context).
(and leave the bit-buffer empty).
* If the bit-buffer is empty, and the next byte starts with 1, read that byte into the bit-buffer.
(After we do this, the next bit we pull from the bit-buffer is that '1' bit).
(Or should we throw away that bit,
so we have 7 bits in the bit-buffer,
and the first one *might* be '0' ?).
* If the bit-buffer is not empty, pull the next bit from the bit-buffer and decide what to do next:
** 0: literal: pull the next full byte (or just nybble?) from the compressed stream and emit it literally on the decompressed output stream.
(and insert those 2 nybbles into the predictor table for this context).
(This is used to emit hi-bit-set bytes, and to emit 0x00 bytes, as well as bytes that are not common in this context).
** 1: emit most-common nybble for this context



FUTURE:
rather than using
least-recently used (LRU)
to update the table of 8
"expected" characters,
try
Adaptive Replacement Cache (ARC).



*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h> // for bool, true, false
#include <iso646.h> // for bitand, bitor, not, xor, etc.
#include <assert.h> // for assert()
#include <ctype.h> // for isprint()

/*
One table each for each of num_contexts different contexts;
(default 16 contexts)
each table containing a list of the most-common byte
in this context.

FUTURE:
try more or fewer context bins ...
if we have *lots* of data,
perhaps more context bins would be better,
to allow
one context with one set of common following words
to be distinguished from
another context that has a different set of following words.
However, if we have relatively little data,
perhaps fewer context bins would be better,
so 2 contexts that map to the same bin
can "share" words that are common to both like " the".
Is there a better way to map "similar contexts" together?
if our microcontroller has very little RAM,
so we only have room for a few contexts, perhaps
* vowels (uppercase and lowercase)
* consonants (uppercase and lowercase)
* period (decimal point), digits, +, and -
* the space character
* the remaining bytes (symbols; tab, newline, other control characters)
If our microcontroller has *very* little RAM,
perhaps we allow this to be scalable to 2 or even only 1 context.

*/
#define COMPILE_TIME_ASSERT(pred) switch(0){case 0:case pred:;}

#define letters_per_context (8)
#define num_contexts (16)
int byte_to_context( char byte ){
    // "clever" code that assumes num_contexts is a power of 2.
    return ((byte>>3) bitand (num_contexts-1));
}
/*
If I used
const int num_contexts = 32;
const int word_indexes = 256;
to define the array sizes,
it would compile fine on
C++ compilers and C compilers that support "-std=c99".
But I use preprocessor "#define" statements instead
so this code can compile without error on older C compilers.
See
http://c-faq.com/ansi/constasconst.html
*/
typedef struct context_table_type {
    char letter[num_contexts][letters_per_context];
    // only used for debug performance monitoring
    int times_used_directly;
} context_table_type;

void initialize_dictionary(
    context_table_type * context_table
){
    int context=0;
    for( context=0; context<num_contexts; context++ ){
        context_table->letter[context][0] = ' ';
        context_table->letter[context][1] = 'e';
        context_table->letter[context][2] = 't';
        context_table->letter[context][3] = 'a';

        context_table->letter[context][4] = 'o';
        context_table->letter[context][5] = 'i';
        context_table->letter[context][6] = 'n';
        context_table->letter[context][7] = 's';
    };
}

void print_as_c_literal( const char * s, int length ){
    bool avoid_hex_digit = false;
    // Would it be better to use full 3-digit octal escapes
    // rather than hex escapes
    // so we don't have to handle the special case
    // when the following character is also a hex digit?
    int line_length = 0;
    int i = 0;
    putchar('"');
    for( ; i < length; i++ ){
        if( 70 <= line_length ){
            printf( "\"\n  \"" );
            line_length = 3;
            avoid_hex_digit = false;
        };
        char c = *s++;
        if( '"' == c ){
            printf( "\\"  "\"" );
            line_length += 2;
            avoid_hex_digit = false;
        }else if( '\\' == c ){
            printf( "\\"  "\\" );
            line_length += 2;
            avoid_hex_digit = false;
        }else if(avoid_hex_digit and isxdigit(c) ){
            // Sometimes the C compiler will eat more than 2 hex digits
            // after a "\x" hex escape code in a C string. See
            // http://stackoverflow.com/questions/2735101/number-of-digits-in-a-hex-escape-code-in-c-c
            putchar( '"' );
            putchar( ' ' );
            putchar( '"' );
            putchar(c);
            line_length += 4;
            avoid_hex_digit = false;
        }else if(isprint(c)){
            /*
            All isprint() characters can represent themselves in a C string,
            except for the above 3 special cases.
            */
            putchar(c);
            line_length++;
            avoid_hex_digit = false;
        }else if( '\n' == c ){
            printf( "\\"  "n" );
            line_length += 2;
            avoid_hex_digit = false;
        }else if( '\t' == c ){
            printf( "\\"  "t" );
            line_length += 2;
            avoid_hex_digit = false;
        }else{
            /* This hexadecimal escape case can handle *every* byte,
            including the NULL character.
            even if we are currently avoiding hex digits.
            The above special cases are just to make
            the output printed source code more human-readable;
            if we eliminated all those special cases
            and always used the hexadecimal escape or octal escape
            for every byte,
            the resulting compiled executable file would be identical.
            */
            printf( "\\"  "x%x%x", ((c >> 4) & 0xf), (c & 0xf) );
            line_length += 4;
            avoid_hex_digit = true;
        };
    };
    putchar('"');
}

/*
print compressed text (or other arbitrary byte string)
for use in, for example, Arduino source code.
*/
void print_as_c_string( const char * s, int length ){
    printf( "char compressed_data = \n" );
    print_as_c_literal( s, length );
    printf( " /* %i bytes. */\n", length );
}

int decompress_nybble(
    context_table_type context_table,
    const char context,
    const char nybble, const char next_nybble,
    char * dest
){
    char output_byte;
    if( nybble bitand 0x08 ){
        // hi bit of nybble set -- it's compressed.
        output_byte = context_table.letter[ (int)(unsigned char)context ][ (int)(nybble bitand 0x07) ];
        *dest = output_byte;
        return 1;
    }else{
        // hi bit of nybble clear -- it's a literal
        output_byte = ((nybble bitand 0x07) << 4) + next_nybble;
        *dest = output_byte;
        return 2;
    };
    assert(0);
    return 0;
}

int update_context(
    context_table_type context_table,
    const char context_byte, const char output_byte
){
    // put output_byte in position 0, moving all other letters
    // but first grab old value that was in position 0
    int context = (int)(unsigned char)context_byte;
    char new_letter = output_byte;
    int position = 0;
    do{
        char old_letter = context_table.letter[context][position];
        context_table.letter[context][position] = new_letter;
        new_letter = old_letter;
        position++;
    }while(
        ( position < letters_per_context ) and
        (new_letter != output_byte)
    );

    assert( output_byte == context_table.letter[context][0] );
    return 0;
}


void
debug_print_dictionary_contents(
    context_table_type context_table
){
    printf( "decompression dictionary: \n" );
    int context = 0;
    for( context=0; context<num_contexts; context++ ){
        int index = 0;
        for( index=0; index<letters_per_context; index++ ){
            printf( "%c", context_table.letter[context][index] );
        };
        printf( "\n" );
    };
}

/*
exhaustively commented version:
While decompressing, print
compressed byte on the left, the "word" of decompressed text decoded from it on the right.
(Optional: bundle up consecutive literals all on one line, rather than one per line).
Something like

   'Hello,': 'Hello,'
   '\x80': ' w'

*/
#define NYBBLES 0xAF /* arbitrarily chosen */
void decompress_bytestring( const char * source, char * dest_original ){
    char * dest = dest_original;
    size_t compressed_length = strlen( source );
    printf( "compressed_length: %zi.\n", compressed_length );
    int compression_type = *source++;
    if( NYBBLES == compression_type ){
        context_table_type context_table;
        initialize_dictionary( &context_table );
        printf("dictionary after first initialization:\n");
        debug_print_dictionary_contents(context_table);
        // first byte copied unchanged, in order to provide context
        printf( "'%c': (%c)", source[0], source[0] );
        *dest++ = *source++;
        int nybble_offset = 0;
        while( *source ){
            int index = (unsigned char)*source++;
            // context is the most recent byte
            int context = byte_to_context( (unsigned char)dest[-1] );
            // FUTURE:
            // rather than splitting up literals pre-emtively here
            // and then later noticing it's a literal
            // and re-assembling it in decompress_nybble,
            // perhaps better to leave source byte
            // as a single unit
            // until we know it's not a literal
            // and needs to be split up.
            int nybble;
            int next_nybble;
            if( 0 == nybble_offset ){
                nybble = (index >> 4) bitand 0xF;
                next_nybble = index bitand 0xF;
            }else{
                nybble = index bitand 0xF;
                next_nybble = (source[1] >> 4) bitand 0xF;
            };
            int nybbles_used = decompress_nybble(
                context_table, context, nybble, next_nybble, dest
            );
            print_as_c_literal( source, 1 );
            printf(": ");
            print_as_c_literal(dest, 1);
            putchar('\n');
            assert( 1 <= nybbles_used );
            nybble_offset += nybbles_used;
            if( nybble_offset >= 2 ){
                source++;
                nybble_offset -= 2;
            };
            assert( nybble_offset < 2 );
            dest++;
        };
        printf("final dictionary:\n");
        debug_print_dictionary_contents(context_table);
    }else{
        // uncompressed literals
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
    };
    *dest = '\0'; // null termination.
    size_t decompressed_length = strlen( dest_original ); // assume null-terminated -- wise?
    printf( "decompressed_length: %zi.\n", decompressed_length );
}

int
compress_byte_index(
    context_table_type * context_table,
    int context, const char * source, char * dest
){
    char s = source[0];
    // If this character
    // is in the context table, compress it.
    // Otherwise emit it as a literal.
    int i = 0;
    do{
        char c = context_table->letter[context][i];
        i++;
    }while(
        (i < letters_per_context) and
        (c != s);
    );


    assert( next_index < 0x80 );
    // FIXME: implement
    selected_index = next_index;
    bytes_eaten = 1;
    // quick test, using a hard-wired dictionary
    if( (' ' == source[0]) and islower(source[1]) ){
        selected_index = 0x80 + source[1];
        bytes_eaten = 2;
    };
    unsigned char byte = source[byte_offset];
    assert( byte < 0x80 );

    assert( 0 != selected_index );
    *dest = selected_index;
    if( dest[0] bitand 0x80 ){
        assert( 1 < bytes_eaten );
    }else{
        assert( 1 == bytes_eaten );
    };
    increment_dictionary_index( context, next_word_index );
    return bytes_eaten;
}


void compress_bytestring( const char * source_original, char * dest_original){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = NYBBLES;

    context_table_type context_table;
    initialize_dictionary( context_table );
    printf("dictionary after first initialization:\n");
    debug_print_dictionary_contents(context_table);
    printf("compressing ...\n");

    *dest++ = compression_type;
    // first byte copied unchanged, in order to provide context
    *dest++ = *source++;
    // int previous_context = 0;
    while( *source ){ // assume null-terminated string -- is this wise?
        int context = byte_to_context( source[-1] );
        int bytes = compress_byte_index(
            &compression_table,
            next_word_index,
            context, source, dest
            );

        /*
        FIXME:
        maintain a dictionary synchronized with the decompressor,
        or find some other way to prune and re-use dictionary indexes
        in a compatible way.
        int tochange = next_word_index[context];
        update_dictionary(previous_context, previous_index, context, index, tochange);
        increment_dictionary_index( context );
        */
        // each compressed index uses 1 byte
        dest++;

        print_as_c_literal( source, bytes );
        assert( 1 <= bytes );
        if( dest[-1] bitand 0x80 ){
            assert( 1 < bytes );
        }else{
            assert( 1 == bytes );
        };
        source += bytes;
        // previous_context = context;
    };
    *dest = '\0'; // null termination.
    printf("table after some compression:\n");
    debug_print_dictionary_contents(context_table);

    size_t source_length = strlen( source_original );
    printf( "source_length: %zi.\n", source_length );
    size_t compressed_length = strlen( dest_original ); // assume null-terminated -- wise?

    if( compressed_length >= source_length ){
        compression_type = LITERAL;
    };

    if( LITERAL == compression_type ){
        printf("incompressible section; copying as literals.");
        source = source_original;
        dest = dest_original;
        // Alas, currently only supports
        // 7-bit data, without high-bit-set.
        // FUTURE:
        // consider something like
        // *dest++ = compression_type;
        // to support 8-bit data.
        assert( source[0] < 128 );
        
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
        *dest = '\0'; // null termination.
    };
}


int
test_compress_byte_index(
    int next_word_index[num_contexts],
    int context, const char * source, char * dest
){
    int bytes_eaten = 0;
    int selected_index = 0;
    int next_index = source[0];
    selected_index = next_index;
    bytes_eaten = 1;
    // quick test, using a hard-wired dictionary
    if( (' ' == source[0]) and islower(source[1]) ){
        selected_index = 0x80 + source[1];
        bytes_eaten = 2;
    };
    *dest = selected_index;
    increment_dictionary_index( context, next_word_index );
    return bytes_eaten;
};

void test_compress_bytestring( const char * source_original, char * dest_original){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    printf("quick test, using a hard-wired dictionary.\n");

    int next_word_index[num_contexts] = {0};
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes] = { };
    initialize_dictionary( dictionary, next_word_index );
    printf("dictionary after first initialization:\n");
    /*
    debug_print_dictionary_contents();
    */
    printf("compressing ...\n");

    *dest++ = compression_type;
    source = source_original;
    // first byte copied unchanged, in order to provide context
    *dest++ = *source++;
    // int previous_context = 0;
    while( *source ){ // assume null-terminated string -- is this wise?
        int context = byte_to_context( dest[-1] );
        int bytes = test_compress_byte_index(
            next_word_index,
            context, source, dest
            );

        // each compressed index uses 1 byte
        assert( 256 >= word_indexes );
        dest++;

        print_as_c_literal( source, bytes );
        assert( 1 <= bytes );
        if( dest[-1] bitand 0x80 ){
            assert( 1 < bytes );
        }else{
            assert( 1 == bytes );
        };
        source += bytes;
        // previous_context = context;
    };
    *dest = '\0'; // null termination.
    /*
    printf("table after some compression:\n");
    debug_print_dictionary_contents();
    */

    size_t source_length = strlen( source_original );
    printf( "source_length: %zi.\n", source_length );
    size_t compressed_length = strlen( dest_original ); // assume null-terminated -- wise?

    if( compressed_length >= source_length ){
        compression_type = LITERAL;
    };

    if( LITERAL == compression_type ){
        printf("incompressible section; copying as literals.");
        source = source_original;
        dest = dest_original;
        *dest++ = compression_type;
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
        *dest = '\0'; // null termination.
    };
}




/*
FIXME:
make sure this works as a fully general compression algorithm
for *any* series of bytes --
in particular,
plaintext that includes all 256 byte.
Add tests that check for each of these categories:
* upper-bit-set bytes
* a series of embedded NULL bytes 0x00 followed by other characters.
* all control characters 0x00...0x2F

By design, this algorithm produces compressed text
that never uses the 0x00 byte,
but it would be nice if it properly handled plaintext
even when the plaintext has embedded 0x00 bytes.

On the other hand, DAV thinks it makes things much easier to test and debug
if every literal byte in the table
(i.e., every compressed index that decodes to a string exactly 2 nybbles long)
was represented by itself in the compressed text.
Perhaps in that case, whenever the plaintext includes 0x00 bytes,
it might be OK to allow the compressed text to use the 0x00 byte,
and use some other technique for indicating end-of-file.
*/



/*
Goal:
compressed text for 8-bit processors
using extremely little program space
and relatively little RAM.

Goal:
a system where example compressed text
can be easily shown on paper,
as a pedagogical tool in the Data Compression wikibook.

(Perhaps also format as a small "signature program",
with compressed text that only uses
characters that can literally represent themselves in a C source string,
and optimized to output plaintext
that only uses letters in the the basic C source character set.
).

*/


void
write_nybble( int nybble, char * dest, bool nybble_offset ){
    assert( ((unsigned)nybble) < 0x10 );
    int old_value = *dest;
    if( 0 == nybble_offset ){
        // overwrite the *first* nybble,
        // while preserving the other nybbble.
        #if little_endian
        int new_value = nybble bitor (old_value bitand 0xf0 );
        #else
        int new_value = ( nybble << 4 ) bitor (old_value bitand 0x0f );
        #endif
        *dest = new_value;
    }else{
        // overwrite the *last* nybble,
        // while preserving the other nybbble.
        #if little_endian
        int new_value = ( nybble << 4 ) bitor (old_value bitand 0x0f );
        #else
        int new_value = nybble bitor (old_value bitand 0xf0 );
        #endif
        *dest = new_value;
    };
}

/*
returns the number of nybbles written to dest.
*/
int decompress_index( int context, int index, char * dest, bool nybble_offset ){
#undef arbitrary
#define arbitrary 1
#if arbitrary
    // special case for literals
    if( is_literal_index( index ) ){
        int nybble = index2nybble( index );
        assert( ((unsigned)nybble) < 0x10 );
        write_nybble( nybble, dest, nybble_offset );
        int nybble_count = 1;
        return nybble_count;
    }else{
        // this index represents a "word",
        // a string of nybbles in the dictionary.
        int previous_index = table[context][index].prefix_word_index;
        int nybble_count = decompress_index( context, previous_index, dest, nybble_offset );
        dest += (nybble_count >> 1);
        nybble_offset ^= (nybble_count bitand 1);
        int nybble = table[context][index].last_letter;
        assert( ((unsigned)nybble) < 0x10 );
        write_nybble( nybble, dest, nybble_offset );
        return 1+nybble_count;
    };
#else
    assert( 0 == table[context][index].next_index );
    while( !is_literal_index( index ) ){
        int prefix_index = table[context][index].prefix_word_index;
        table[context][prefix_index].next_index = index;
        index = prefix_index;
    };
    assert( is_literal_index( index ) );
    while( table[index].next_index ){
        int nybble = table[context][index].last_letter;
        assert( ((unsigned)nybble) < 0x10 );
        write_nybble( nybble, dest, nybble_offset );
        int next_index = table[context][index].next_index;
        table[context][index].next_index = 0;
        index = next_index;
        if(nybble_offset){ dest++ };
        nybble_offset ^= 1;
    };
    int nybble = table[context][index].last_letter;
    assert( ((unsigned)nybble) < 0x10 );
    write_nybble( nybble, dest, nybble_offset );
#endif
};

bool
isprintable( const char * s ){
    while(*s){
        if( !isprint( *s ) ){
            return false;
        };
        s++;
    };
    return true;
};

/*
print all the data in the table,
in a relatively human-readable format.
*/
void
debug_print_table_contents(){
    printf( "decompression dictionary: " );
    int context = 0;
    for( context=0; context<num_contexts; context++ ){
        int index = 0;
        for( index=0; index<word_indexes; index++ ){
#if only_high_bit_set_is_word
            int value = index | 0x80;
#else
            int value = index;
#endif
            if( is_literal_index(index) ){
                    /*
                    printf( "index: 0x%x ", index );
                    printf( "(literal)" );
                    */
            }else{
                // picking only the lower 5 bits
                // as the context means that
                // we don't know exactly the full byte context ...
                // so print the uppercase letters that
                // map to this context.
                // (probably the lowercase letters and space
                // are more likely ...)
                int context_letter = context + '@';
                char dest[256]; // longer than the longest possible string.
                int nybbles = decompress_index( context, index, dest, 0 );
                if( (2 == nybbles) and ( (unsigned char)dest[0] == value ) ){
                    // same as default
                }else{
                    printf( "index: 0x%x ", index );
                    printf( "[0x%x = %c]", context, context_letter);
                    putchar('[');
                    print_as_c_literal( dest, (nybbles+1)/2 );
                    putchar(']');
                    Word_in_nybble_table_type word = table[context][index];
                    assert( 0 == word.next_index );
                    if( word.recently_used ){ printf( "(recent)" ); };
                    int prefix_index = word.prefix_word_index;
                    assert( 0 == table[context][prefix_index].leaf );
                    printf( "\n" );
                }; 
            };
        };
    };
}

void
increment_table_index( int context, int next_word_index[num_contexts]  ){
    /*
    Normally this is simply
        next_word_index[context]++;
    except for:
    * wrap-around
    * only prune leaves.
    */
    bool leaf = true;
    int next_index = next_word_index[context];
    do{
        next_index++;
        #define wraptype only_hi_bit_set
        #if only_hi_bit_set == wraptype
            if( 0x100 <= next_index ){
                // wrap around from compressed index "0xff" to "0x80".
                next_index = 0x80;
            };
        #elif every_byte_but_null == wraptype
            if( 0x100 <= next_index ){
                // wrap around, skipping NULL
                next_index = 1;
            };
            // skip over the isliteral() nybbles
            if( (0x10 <= next_index) and (next_index <= 0x1f) ){
                next_index = 0x20;
            };
        #elif only_c_source_characters
            if( 0x7f <= next_index ){
                // wrap around, skipping NULL
                next_index = 0x20;
            };
            // FIXME: skip over the isliteral() nybbles
        #else
            #error "no wrap type set. Sorry."
        #endif
        /* FIXME:
        only prune leaves.
        If this current index is *not* a leaf,
        try the next one.
        FIXME:
        how to avoid infinite loop here?
        leaf = table[context][next_index].leaf;
        */
    }while(!leaf);
    if( ! table[context][next_index].leaf ){
        printf( "context %x, index %x, is not a leaf.\n", context, next_index );
    };
    /*
    assert( table[context][next_index].leaf );
    */

    next_word_index[context] = next_index;
};

/*
Given the context and index of 2 consecutive indexes
in the compressed text.
*/
void
update_table(int context, int index, int next_context, int next_index, int tochange){
    /*
    int tochange = next_word_index[context];
    */

    if( tochange != next_index ){
        // normal case

    // FIXME: we also do this in decompress_index;
    // it would be better to do this only once, either here or there.
    while( !is_literal_index( next_index ) ){
        int prefix_index = table[next_context][next_index].prefix_word_index;
        next_index = prefix_index;
    };

    }else{
        // handle the "special case" LZW exception,
        // as mentioned by
        // http://michael.dipperstein.com/lzw/#example3
        // https://www.cs.duke.edu/csed/curious/compression/lzw.html#decompression
        // https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Welch#Decoding_2
        // http://www.perlmonks.org/?node_id=270016
        // http://stackoverflow.com/questions/10450395/lzw-decompression-algorithm
        // etc.
        // DAV:
        // are there any other special cases in this
        // 
        printf("(... handling LZW special case ...)");


    };
    assert( is_literal_index( next_index ) );
    char first_nybble_of_next_word = index2nybble(next_index);
    assert( ( ((unsigned)first_nybble_of_next_word) < 0x10 ) );

    table[context][tochange].prefix_word_index = index;
    table[context][tochange].last_letter = first_nybble_of_next_word;
    table[context][index].leaf = false;

}

void decompress( const char * source, char * dest_original ){
    char * dest = dest_original;
    size_t compressed_length = strlen( source );
    printf( "compressed_length: %zi.\n", compressed_length );
    int compression_type = *source++;
    if( EIGHT_BIT_PRUNED == compression_type ){
        /* FIXME: */
        int next_word_index[num_contexts] = {0};
        initialize_table( next_word_index );
        bool nybble_offset = 0;
        int previous_index = source[0];
        // first byte copied unchanged, in order to provide context
        *dest++ = *source++;
        int previous_context = ' ';
        while( *source ){
            int index = *source++;
            // should context be the most recent complete aligned byte,
            // or the most recent (possibly unaligned) 2 nybbles?
            int context = byte_to_context( dest[-1] );
            /* Add a word to the dictionary:
               the previous word,
               and the first nybble of the next word.
               We assume that the compressor
               *would* have used *that* word if it had been available,
               so clearly that word is *not* already in the dictionary.
               Add entry just before decompressing it,
               to handle the LZW special case.
            */
            int tochange = next_word_index[context];
            update_table(previous_context, previous_index, context, index, tochange);
            increment_table_index( context, next_word_index );
            int nybbles = decompress_index( context, index, dest, nybble_offset );
            assert( 1 <= nybbles );

            
            dest += ((nybbles+nybble_offset) >> 1);
            nybble_offset ^= (nybbles bitand 1);
            previous_context = context;
            previous_index = index;
        };
        debug_print_table_contents();
    }else if( LITERAL == compression_type ){
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
    }else{
        printf("invalid compressed data");
    };
    *dest = '\0'; // null termination.
    size_t decompressed_length = strlen( dest_original ); // assume null-terminated -- wise?
    printf( "decompressed_length: %zi.\n", decompressed_length );
}


/*
compression table:
for every possible current_index,
(i.e., some prefix string)
a list:
For each of 16 possible next-nybbles,
if that longer string exists in the dictionary,
the table indicates what index 
(i.e., some slightly longer string)
represents that longer string.
Currently we're reserving 0x00
to indicate "that longer string does not yet exist in the dictionary".
FUTURE:
Consider using a completely out-of-band indicator
or some other reserved index, perhaps a "literal" index.
The compression_table[]
needs to be synchronized to the decompression table[].
*/
void initialize_compression_table(
    int compression_table[num_contexts][word_indexes][16]
){
    int context=0;
    for( context=0; context<num_contexts; context++ ){
        // initialize all the indexes,
        // even if we have more than 256 of them.
        int index=0;
        for( index=0; index<word_indexes; index++ ){
/* FIXME:
#if only_high_bit_set_is_word
            int value = index | 0x80;
#else
*/
            // is it better to go big-endian or little-endian?
            #if little_endian
            int second_nybble = (index >> 4) bitand 0x0f;
            #else
            int second_nybble = (index) bitand 0x0f;
            #endif
            int nybble = 0;
            for( nybble = 0; nybble<16; nybble++ ){
                if( is_literal_index( index ) ){
                    compression_table[context][index][nybble] = nybble2index(second_nybble);
                }else{
                    compression_table[context][index][nybble] = 0;
                };
            };
        };
    };
};

int get_nybble( const char * source, bool nybble_offset ){
    #if little_endian
    int first_nybble = (*source) bitand 0x0f;
    int second_nybble = ((*source) >> 4) bitand 0x0f;
    #else
    int first_nybble = ((*source) >> 4) bitand 0x0f;
    int second_nybble = (*source) bitand 0x0f;
    #endif
    if( nybble_offset ){
        int nybble = nybble2index( second_nybble );
        return nybble;
    }else{
        int nybble = nybble2index( first_nybble );
        return nybble;
    };
};

int
compress_index(
    int compression_table[num_contexts][word_indexes][16],
    int next_word_index[num_contexts],
    int context, const char * source, char * dest, bool original_nybble_offset
){
    int nybble_offset = (unsigned) original_nybble_offset;
    // normal compression
    int nybbles_eaten = 0;
    int selected_index = 0;
    int next_index = nybble2index( get_nybble( source + (nybble_offset >> 1), nybble_offset bitand 1 ) );
    do{
        nybble_offset++;
        nybbles_eaten++;
        int nybble = get_nybble( source + (nybble_offset >> 1), nybble_offset bitand 1 );
        selected_index = next_index;
        next_index = compression_table[context][selected_index][nybble];
    }while( next_index );
    *dest = selected_index;
    increment_table_index( context, next_word_index );
    return nybbles_eaten;
};

void
debug_print_nybbles( const char * source, int nybbles ){
    int i=0;
    int bytes = (nybbles+1)/2;
    putchar( '(' );
    // print all the bytes that are completely or partially covered.
    for( ; i<bytes; i++ ){
        putchar( source[i] );
    };
    putchar( ')' );
}

void compress( const char * source_original, char * dest_original ){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    int compression_table[num_contexts][word_indexes][16] = { };

    int next_word_index[num_contexts] = {0};
    initialize_table( next_word_index );
    printf("table after first initialization:\n");
    debug_print_table_contents();
    printf("compressing ...\n");

    *dest++ = compression_type;
    /* FIXME: implement */
        source = source_original;
        // first byte copied unchanged, in order to provide context
        *dest++ = *source++;
        bool nybble_offset = 0;
        // int previous_context = 0;
        while( *source ){ // assume null-terminated string -- is this wise?
            int context = byte_to_context( dest[-1] );
            int nybbles = compress_index(
                compression_table,
                next_word_index,
                context, source, dest, nybble_offset
                );

            // each compressed index uses 1 byte
            assert( 256 >= word_indexes );
            dest++;

            debug_print_nybbles( source, nybbles+nybble_offset );
            assert( 1 <= nybbles );
            source += ((nybbles+nybble_offset) >> 1);
            nybble_offset ^= (nybbles bitand 1);
            // previous_context = context;
        };
    *dest = '\0'; // null termination.
    printf("table after some compression:\n");
    debug_print_table_contents();

    size_t source_length = strlen( source_original );
    printf( "source_length: %zi.\n", source_length );
    size_t compressed_length = strlen( dest_original ); // assume null-terminated -- wise?

    if( compressed_length >= source_length ){
        compression_type = LITERAL;
    };

    if( LITERAL == compression_type ){
        printf("incompressible section; copying as literals.");
        source = source_original;
        dest = dest_original;
        *dest++ = compression_type;
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
        *dest = '\0'; // null termination.
    };
}


/*
mock test used as a quick test of the decoder
*/
int
test_byte_compress_index(
    int compression_table[num_contexts][word_indexes][16],
    int next_word_index[num_contexts],
    int context, const char * source, char * dest, bool original_nybble_offset
){
    *dest = *source;
    increment_table_index( context, next_word_index );
    return 2;
};

void
test_byte_compress( const char * source_original, char * dest_original ){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    int compression_table[num_contexts][word_indexes][16] = { };

    int next_word_index[num_contexts] = {0};
    initialize_table( next_word_index );
    printf("table after first initialization:\n");
    debug_print_table_contents();
    printf("compressing ...\n");

    *dest++ = compression_type;
    /* FIXME: implement */
        source = source_original;
        // first byte copied unchanged, in order to provide context
        *dest++ = *source++;
        bool nybble_offset = 0;
        // int previous_context = 0;
        while( *source ){ // assume null-terminated string -- is this wise?
            int context = byte_to_context( dest[-1] );
            int nybbles = test_byte_compress_index(
                compression_table,
                next_word_index,
                context, source, dest, nybble_offset
                );

            // each compressed index uses 1 byte
            assert( 256 >= word_indexes );
            dest++;

            debug_print_nybbles( source, nybbles+nybble_offset );
            assert( 1 <= nybbles );
            source += ((nybbles+nybble_offset) >> 1);
            nybble_offset ^= (nybbles bitand 1);
            // previous_context = context;
        };
    *dest = '\0'; // null termination.
    printf("table after some compression:\n");
    debug_print_table_contents();

    size_t source_length = strlen( source_original );
    printf( "source_length: %zi.\n", source_length );
    size_t compressed_length = strlen( dest_original ); // assume null-terminated -- wise?

    if( compressed_length >= source_length ){
        compression_type = LITERAL;
    };

    if( LITERAL == compression_type ){
        printf("incompressible section; copying as literals.");
        source = source_original;
        dest = dest_original;
        *dest++ = compression_type;
        while( *source ){ // assume null-terminated string -- is this wise?
            *dest++ = *source++;
        };
        *dest = '\0'; // null termination.
    };
}


int
test_nybble_compress_index(
    int next_word_index[num_contexts],
    int context, const char * source, char * dest, bool original_nybble_offset
){
    int nybble_offset = (unsigned) original_nybble_offset;
    *dest = get_nybble( source, nybble_offset );
    return 1;
};


void test_nybble_compress( const char * source_original, char * dest_original ){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;
    int next_word_index[num_contexts] = {0};

    *dest++ = compression_type;
    /* FIXME: implement */
        source = source_original;
        // first byte copied unchanged, in order to provide context
        *dest++ = *source++;
        bool nybble_offset = 0;
        while( *source ){ // assume null-terminated string -- is this wise?
            int nybbles = test_nybble_compress_index(
                next_word_index,
                0, source, dest, nybble_offset
                );
            dest++;

            debug_print_nybbles( source, nybbles+nybble_offset );
            assert( 1 <= nybbles );
            source += ((nybbles+nybble_offset) >> 1);
            nybble_offset ^= (nybbles bitand 1);
        };
    *dest = '\0'; // null termination.
}

int main( void ){
    char compressed_text[1000] = " Hello, world.";
    printf( "%s", compressed_text );
    char decompressed_text[100];

    decompress_bytestring( compressed_text, decompressed_text );
    printf("quick test b: [%s]\n", decompressed_text);

    decompress( compressed_text, decompressed_text );
    printf("quick test n: [%s]\n", decompressed_text);

    char text[100] =
        "Hello, world. "
        "This is a test. "
        "This is only a test. "
        "Banana banana banana banana. "
        "";
    int text_length = strlen(text);
    printf("testing with [%s].\n", text );

    printf("quick test with test_compress_bytestring ...\n");
    test_compress_bytestring( text, compressed_text );
    print_as_c_string( compressed_text, strlen(compressed_text) );
    assert( strlen( compressed_text ) <= 70 );
    decompress_bytestring( compressed_text, decompressed_text );
    printf("decompressed: [%s]\n", decompressed_text);
    if( memcmp( text, decompressed_text, text_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };

    printf("testing compress_bytestring ...\n");
    compress_bytestring( text, compressed_text );
    print_as_c_string( compressed_text, strlen(compressed_text) );
    assert( strlen( compressed_text ) <= 70 );
    printf("testing decompress_bytestring ...\n");
    decompress_bytestring( compressed_text, decompressed_text );
    printf("decompressed: [%s]\n", decompressed_text);
    if( memcmp( text, decompressed_text, text_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };

// FIXME: do exhaustive test of both bytestring and nybblestring compression.

    test_nybble_compress( text, compressed_text );
    /*
    Except for
    the "8-bit pruned" header, and
    the first byte passed straight through verbatim,
    this test produces 2 "compressed" bytes for each original byte;
    */
    assert( 0 != compressed_text[ 2*(text_length-1) + 1 ] );
    assert( 0 == compressed_text[ 2*(text_length-1) + 2 ] );
    print_as_c_string( compressed_text, 2*text_length+1 );
    /*
    decompress( compressed_text, decompressed_text );
    printf("decompressed: [%s]\n", decompressed_text);
    if( memcmp( text, decompressed_text, text_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };
    */

    /*
    For the first 128 bytes or so,
    the hi-bit-clear bytes represent themselves --
    quick test of the decoder.
    */
    test_byte_compress( text, compressed_text );
    print_as_c_string( compressed_text, strlen(compressed_text) );
    decompress( compressed_text, decompressed_text );
    printf("decompressed: [%s]\n", decompressed_text);
    if( memcmp( text, decompressed_text, text_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };

    compress( text, compressed_text );
    print_as_c_string( compressed_text, strlen(compressed_text) );
    decompress( compressed_text, decompressed_text );
    printf("decompressed: [%s]\n", decompressed_text);
    if( memcmp( text, decompressed_text, text_length ) ){
        printf("Error: decompressed text doesn't match original text.\n");
        printf("[%s] original\n", text);
        printf("[%s] decompressed\n", decompressed_text);
    }else{
        printf("Successful test.\n");
    };



    return 0;
}

/* vim: set shiftwidth=4 expandtab ignorecase smartcase incsearch softtabstop=4 background=dark : */

