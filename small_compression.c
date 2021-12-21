/* small_compression.c
WARNING: version 2015.00.00.03-alpha : extremely rough draft.
2015-05-24: David Cary started.

a data compression algorithm for small embedded systems.

The compressed file is a series of bytes.
Each byte represents a "word" (a series of nybbles).
LZW-like algorithm, but with more context and better pruning.

The previous byte of context
indicates which "dictionary" of known words to look up the next word in.

Trim all leaf nodes periodically.

Initialize all dictionaries so, initially, every byte represents itself.

Avoid using the 0x00 byte in the compressed text.

*/

/*
FUTURE:
consider using
https://en.wikipedia.org/wiki/C_data_types#Fixed-width_integer_types
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h> // for bool, true, false
#include <iso646.h> // for bitand, bitor, not, xor, etc.
#include <assert.h> // for assert()
#include <ctype.h> // for isprint()

enum algorithm {
    LITERAL = ' ',
    ISPRINT_IS_ALWAYS_LITERAL = 0x1f,
    COMPRESSED_TEXT_IS_PRINTABLE = '_',
    EIGHT_BIT_PRUNED = 8,
};


/*
One table each for each of num_contexts different contexts;
(default 32 contexts)
each table containing a list of words in a compressed format.
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

#define num_contexts (32)
int byte_to_context( char byte ){
    // "clever" code that assumes num_contexts is a power of 2.
    return (byte bitand (num_contexts-1));
}

#define word_indexes (256)
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
/*
word_indexes = 256 lets each word be represented by a byte.
FUTURE: experiment with word_indexes = 0x1000
so each word can be represented by 12 bits, like GIF-variant LZW.
FUTURE: experiment with starting out with short 8-bit indexes
and gradually expanding as necessary, like GIF-variant LZW.
FUTURE:

Consider storing the "words" in the dictionary more directly,
for quicker copies ...
so perhaps some buffer that uses about the same (less?) RAM
that contains all the words in the dictionary,
and this structure
points to the first byte (nybble?)
of that word in the buffer and its length.
...
Traditional LZW only makes words by adding letters to the end of the word,
and the second-simplest form of pruning only deletes letters
from the end of the word;
perhaps such a buffer would make it easier to add a letter
to the beginning of a word
and prune by deleting letter from the beginning of a word.

A few pruning strategies:
* after we use up the last index in some context,
clear and re-initialize *just that context*
(all other contexts continue filling up right where they left off)
* after we use up the last index in some context,
prune *all* the leaf words in that context to open up at least one empty slot,
then start filling up the empty slots again.
* Dictionary always full:
use some cache algorithm -- perhaps the clock algorithm --
to empty up one leaf word as needed.

*/
typedef struct Word_in_byte_dictionary_type {
    int prefix_word_index;
    char last_letter;
    bool leaf;
    bool recently_used;
    // only used for debug performance monitoring
    int times_used_directly;
    int times_used_indirectly;
} Word_in_byte_dictionary_type;

#define dictionary_indexes (0x7f)

/*
FUTURE:
perhaps something like
"diagnostic push" and "diagnostic pop"
explained on page
https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
could be used with
#pragma GCC diagnostic ignored "-Wtype-limits"
to disable
the following kind of "error" *only* inside assert() statements?

When compiling with -Werror,
How do I disable the
"error: comparison is always true due to limited range of data type [-Werror=type-limits]"
in code like
            char letter = index;
            assert( letter < 0x80 );
            assert( 0 < letter );
?
(I want to leave that error,
or at least a warning,
turned on for this kind of warning *outside* asserts,
but ideally I don't even want the warning
*inside* asserts.

I couldn't figure out how to disable this
*only* inside assert() statements,
so I'm reducing it from an error to a warning
with this pragma:
*/
#pragma GCC diagnostic warning "-Wtype-limits"

void initialize_dictionary(
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes],
    int next_word_index[num_contexts]
){
    int context=0;
    for( context=0; context<num_contexts; context++ ){
        int index=0;
        for( index=0; index<dictionary_indexes; index++ ){
            // pure byte-oriented -- big-endian or little-endian is irrelevant.
            // apparently we never use these escape codes when
            // the plain text is normal 7-bit ASCII text.
            #define escape_code(x) ( 0x1f == ((x) bitor 0xf) )
            dictionary[context][index].prefix_word_index = ' ';
            char letter = index;
            if( 0 == letter ){ letter = 'x'; }; // TODO: remove after debugging.
            assert( letter < 0x80 );
            assert( 0 < letter );
            dictionary[context][index].last_letter = letter;
            dictionary[context][index].leaf = true;
            dictionary[context][index].recently_used = false;
            dictionary[context][index].times_used_directly = 0;
            dictionary[context][index].times_used_indirectly = 0;
        };
        next_word_index[ context ] = 0;
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

// returns the number of bytes written to dest.
int decompress_byte_index(
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes],
    const int context, int index, char * dest
){
    char reversed_word[128];
    int i=0;
    assert( 0x00 != index );

    while( index bitand 0x80 ){
        int prefix_index = dictionary[context][index - 0x80].prefix_word_index;
        if( 0x80 <= prefix_index ){
            assert( !dictionary[context][prefix_index-0x80].leaf );
        };
        char letter = dictionary[context][index - 0x80].last_letter;
        assert( letter < 0x80 );
        if( letter <= 0 ){
            printf("context: 0x%x, index: 0x%x", context, index);
            printf("letter: 0x%x\n", letter);
        };
        assert( 0 < letter );
        reversed_word[i++] = letter;
        index = prefix_index;
    };
    assert( !(index bitand 0x80) );
    reversed_word[i++] = index;
    // This if() supports hi-bit-set and 0x00 bytes in the plaintext.
    // It may not be necessary if those bytes never occur in the plaintext.
    if( 0x00 == index ){
        printf( "test: special case: dictionary chain ends with 0x00.\n" );
        i--;
    };
    const int bytes_written = i;
    while( i ){
        char letter = reversed_word[--i];
        // FIXME: remove this assert when plaintext includes hi-bit-set bytes.
        assert( letter < 0x80 );
        assert( 0 < letter );
        *dest = letter;
        dest++;
    };

    return bytes_written;
}

void
debug_print_dictionary_entry(
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes],
    int context, int index
){
        // picking only the lower 5 bits
        // as the context means that
        // we don't know exactly the full byte context ...
        // so print the uppercase letters that
        // map to this context.
        // (probably the lowercase letters and space
        // are more likely ...)
        int context_letter = context + '@';
            // int value = index | 0x80;
                char dest[256]; // longer than the longest possible string.
                Word_in_byte_dictionary_type word = dictionary[context][index-0x80];
                int bytes = decompress_byte_index( dictionary, context, index, dest );
                if( (2 == bytes) and
                    ( (unsigned char)dest[0] == ' ' ) and
                    ( (unsigned char)dest[1] == index - 0x80 )
                ){
                    // same as default
                }else{
                    printf( "index: 0x%x ", index );
                    printf( "[0x%x = %c]", context, context_letter);
                    putchar('[');
                    print_as_c_literal( dest, bytes );
                    putchar(']');
                    if( word.recently_used ){ printf( "(recent)" ); };
                    int prefix_index = word.prefix_word_index;
                    assert( 0 < prefix_index );
                    if( prefix_index < 0x80 ){
                        assert( isprint( (char)prefix_index ) );
                    }else{
                        assert( !dictionary[context][prefix_index-0x80].leaf );
                    };
                    printf( "\n" );
                }; 
}

void
debug_print_dictionary_contents(
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes]
){
    printf( "decompression dictionary: \n" );
    int context = 0;
    for( context=0; context<num_contexts; context++ ){
        int index = 0;
        for( index=0x80; index<(0x80+dictionary_indexes); index++ ){
            debug_print_dictionary_entry( dictionary, context, index );
        };
    };
}

void
increment_dictionary_index( int context, int next_word_index[num_contexts] ){
    // FIXME: loop until we hit a leaf node.
    int i = next_word_index[context] + 1;
    if( dictionary_indexes <= i ){
        i = 0;
    };
    next_word_index[context] = i;
}
/*
Given the context and index of 2 consecutive indexes
in the compressed text.
*/
void
update_dictionary(
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes],
    int context, int index, int next_context, int next_index, int tochange
){
    /*
    int tochange = next_word_index[context];
    */
    unsigned char first_byte_of_next_word = 0xff;

    bool special_case = (tochange == next_index) and (context = next_context);
    if( !special_case ){
        // normal case

    // FIXME: we also do this in decompress_byte_index;
    // it would be better to do this only once, either here or there.
    while( next_index bitand 0x80 ){
        assert( next_index >= 0x80 );
        assert( next_index < 0xff );
        int prefix_index = dictionary[next_context][next_index - 0x80].prefix_word_index;
        next_index = prefix_index;
    };
    assert( !(next_index bitand 0x80) );
    first_byte_of_next_word = next_index;

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
        printf("(... handling LZW special case ...)\n");

        assert(0); // unimplemented

    };

    assert( first_byte_of_next_word < 0x80 );

    printf("context: %c, index: 0x%x, last_letter: %c.\n", (char)('@'+context), index, first_byte_of_next_word);
    dictionary[context][tochange].prefix_word_index = index;
    dictionary[context][tochange].last_letter = first_byte_of_next_word;
    if( index >= 0x80 ){
        dictionary[context][index - 0x80].leaf = false;
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
void decompress_bytestring( const char * source, char * dest_original ){
    char * dest = dest_original;
    size_t compressed_length = strlen( source );
    printf( "compressed_length: %zi.\n", compressed_length );
    int compression_type = *source++;
    if( EIGHT_BIT_PRUNED == compression_type ){
        int next_word_index[num_contexts] = {0};
        Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes] = {0};
        initialize_dictionary( dictionary, next_word_index );
        printf("dictionary after first initialization:\n");
        debug_print_dictionary_contents(dictionary);
        // first byte copied unchanged, in order to provide context
        int previous_index = (unsigned char)source[0];
        printf( "'%c': (%c)", source[0], source[0] );
        *dest++ = *source++;
        int previous_context = ' ';
        while( *source ){
            int index = (unsigned char)*source++;
            // context is the most recent byte
            int context = byte_to_context( (unsigned char)dest[-1] );
            /* Add a word to the dictionary:
               the previous word,
               and the first byte of the next word.
               We assume that the compressor
               *would* have used *that* word if it had been available,
               so clearly that word is *not* already in the dictionary.
            */
            int tochange = next_word_index[context];
            update_dictionary(dictionary, previous_context, previous_index, context, index, tochange);
            increment_dictionary_index( context, next_word_index );
            int bytes = decompress_byte_index( dictionary, context, index, dest );
            print_as_c_literal( source, 1 );
            printf(": ");
            print_as_c_literal(dest, bytes);
            putchar('\n');
            assert( 1 <= bytes );
            
            dest += bytes;
            previous_context = context;
            previous_index = index;
        };
        debug_print_dictionary_contents(dictionary);
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

int
compress_byte_index(
    int compression_table[num_contexts][word_indexes][0x80],
    int next_word_index[num_contexts],
    int context, const char * source, char * dest
){
    // FIXME: perhaps print a debug message in the "LZW special case".
    int byte_offset = 0;
    // normal compression
    int bytes_eaten = 0;
    int selected_index = 0;
    int next_index = source[0];
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
    /*
    bytes_eaten = 0;
    unsigned char byte = 0xff;

    // search through the tree
    do{
        byte = source[byte_offset];
        / *
        FIXME:
        remove this assert when compressing hi-bit-set bytes.
        * /
        assert( byte < 0x80 );
        selected_index = next_index;
        next_index = compression_table[context][selected_index][byte];
        assert( (0 == next_index) or (0x80 bitand next_index) );
        byte_offset++;
        bytes_eaten++;
    }while( next_index );
    // add a new leaf to the tree
    assert( 0 == next_index );
    */
    assert( byte < 0x80 );
    /*
    assert( 0 == compression_table[context][selected_index][byte] );
    */
    compression_table[context][selected_index][byte] = next_word_index[context] + 0x80;

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

void
initialize_compression_dictionary(
    int compression_table[num_contexts][word_indexes][0x80]
){
    /* optional special case: spaces */
    int context = 0;
    for(; context<num_contexts; context++){
        int lowercase_letter = 'a';
        for(; lowercase_letter <= 'z'; lowercase_letter++){
            compression_table[context][lowercase_letter][' '] = lowercase_letter+0x80;
        };
    };
    /* end option */
}

void compress_bytestring( const char * source_original, char * dest_original){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    int compression_table[num_contexts][word_indexes][0x80] = {0};
    /*
    FIXME: the 0x80 assumes that
    the plaintext contains only printable characters.
    Extend to allow all possible bytes in the uncompressed text,
    by either
    (a) bumping from 0x80 to 0x100, or
    (b) reducing from (0x80) to (0x10 * 2), with a two-level nybble lookup
    (c) some other reduced-memory variant
    */

    initialize_compression_dictionary( compression_table );

    int next_word_index[num_contexts] = {0};
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes] = {0};
    initialize_dictionary(dictionary, next_word_index);
    printf("dictionary after first initialization:\n");
    debug_print_dictionary_contents(dictionary);
    printf("compressing ...\n");

    *dest++ = compression_type;
    source = source_original;
    // first byte copied unchanged, in order to provide context
    *dest++ = *source++;
    // int previous_context = 0;
    while( *source ){ // assume null-terminated string -- is this wise?
        int context = byte_to_context( source[-1] );
        int bytes = compress_byte_index(
            compression_table,
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
    printf("table after some compression:\n");
    debug_print_dictionary_contents(dictionary);

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
}

void test_compress_bytestring( const char * source_original, char * dest_original){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    printf("quick test, using a hard-wired dictionary.\n");

    int next_word_index[num_contexts] = {0};
    Word_in_byte_dictionary_type dictionary[num_contexts][dictionary_indexes] = {0};
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



typedef struct Word_in_nybble_table_type {
    int prefix_word_index;
    char last_letter;
    int next_index; // only temporarily used inside decompress_index
    bool leaf;
    bool recently_used;
    /*
    // only used for debug performance monitoring
    int times_used_directly;
    int times_used_indirectly;
    */
} Word_in_nybble_table_type;
/*
FIXME:
move next_index
into a temporary array inside decompress_index?
It only needs to be as long as the longest word ...
so with a 128-entry table, max 128 indexes,
and we only need it briefly ...
and perhaps if we somehow guarantee that the longest wordlength
is, say, 32 indexes, it really only needs to be 32 indexes long.
*/

Word_in_nybble_table_type table[num_contexts][word_indexes] = {0};

void initialize_table( int next_word_index[num_contexts] ){
    /* Initialize so that normal ASCII text
       (which occasionally uses carriage return 0x0D and line feed 0x0A)
       can be (initially) represented by itself.
    */
    int context=0;
    for( context=0; context<num_contexts; context++ ){
        // initialize only the 256 bytes, even if
        // we have 12-bit word indexes
        int index=0;
        for( index=0; index<word_indexes; index++ ){
            // is it better to go big-endian or little-endian?
            #define little_endian 1
            #if little_endian
            int first_nybble = (index) bitand 0x0f;
            int second_nybble = (index >> 4) bitand 0x0f;
            #else
            int first_nybble = (index >> 4) bitand 0x0f;
            int second_nybble = (index) bitand 0x0f;
            #endif
            #define nybble2index(x) ((x) bitor 0x10)
            #define index2nybble(x) ((x) bitand (~0x10))
            #define is_literal_index(x) ( 0x1f == ((x) bitor 0xf) )
            assert( ((unsigned)first_nybble) < 0x10 );
            table[context][index].prefix_word_index = nybble2index(first_nybble);
            assert( ((unsigned)second_nybble) < 0x10 );
            table[context][index].last_letter = second_nybble;
            table[context][index].next_index = 0;
            table[context][index].leaf = true;
            if( is_literal_index( index ) ){
                // the literal nybbles
                // are never pruned, and in that sense
                // are not ever considered to be a leaf.
                table[context][index].leaf = false;
            };
            table[context][index].recently_used = true;
        };
        /* "Indexes" 0x10...0x1f should never be looked up in this table.
           Those indexes represent the literal nybbles 0x0...0xf.
           On low-RAM microprocesors, is there a good way
           to avoid spending RAM bytes on these constants?
        */
        /*
        start at high-word-set,
        so we're guaranteed that ASCII text
        where all bytes used in normal 7-bit ASCII text
        represent themselves for
        at least 128 words per context.
        */
        next_word_index[ context ] = 0x80;
        
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

* We periodically "prune" certain indexes -- typically bytes
that are never or rarely used in the plaintext --
and free them up to represent words.
* Sometimes we need to emit a plaintext byte
that isn't in the table.
This happens when the plaintext contains one of the special compressed bytes
(0x00 or 0x10..0x1f).
This also happens after we "prune" a byte and start using it
(in the compressed text) to represent a longer word.

Currently we use this very simple approach
that emits 2 bytes
to represent such literal bytes:
* emit one of the special bytes 0x10..0x1f
to represent the high nybble of the literal byte.
(The *low* nybble of these special bytes becomes the *high* nybble).
When we hardwire the low-ASCII isprint()+DEL characters 0x20..0x7f
to always represent themselves,
this nybble indicates
0x10: 0x00 and other control characters in the plaintext
0x11: 0x10..0x1f control characters in the plaintext
0x12..0x17: should never occur in the compressed text (?);
perhaps we could re-use those bytes as a higher-level escape for other stuff?
0x18..0x1f: high-bit-set characters in the plaintext.
* emit a byte whose low nybble matches the low nybble of the literal byte,
discarding and wasting its high 4 bits.
(perhaps '0' + low_nybble? for relatively easy printable readability?)
(perhaps 0x80 + low_nybble,
so printable characters in the compressed text
*always* represent themselves?
0x81 already represents a variety of things
depending on context,
and immediately after these special bytes 0x10..0x1f,
we can consider this a special context
where 0x81 is always hard-wired to represent 0x01.
)
(perhaps 0x10 + low_nybble,
so the bytes 0x10..0x1f always represent a nybble of an expanded literal byte;
if the plaintext only contains normal 7-bit ASCII text,
and we hardwired the low-ASCII isprint+DEL+CR+NL to represent themselves,
then none of the bytes 0x00 or 0x10..0x1f should ever occur in the compressed text.
and the high-bit-set bytes always represent words longer than 1 byte.
)
(perhaps simply literally store the plaintext byte as-is
in the compressed text ... unless it is the 0x00 byte).

The compressed format for each block is:
* a single "compression type" byte
* a series of nonzero indexes (initially bytes) decompressed as above
* a zero end-of-block terminator.

A more useful archiving tool might have a format something like
* some sort of FourCC or other file format identifier
* The semantic version number of this format
(to enable new software to be backward compatible
with files generated by older software ...
and perhaps some limited amount of forward compatibility)
* various metadata -- filename, creation time, overall checksum, etc.
* A series of blocks that decompress to pieces of the file.
* perhaps wrap each block to start with a 32-bit "length of this compresse block" and "length of this block, after decompression"
(so a parallel processor can skip ahead to the next block,
and decompress multiple blocks parallel)
and follow the block with local block checksum.
* perhaps a couple of RAID-6-like ECC blocks,
so if one of the local checksums fails,
perhaps the data can be recovered
and the overall checksum will confirm it is now good.



Is there some way to get better compression
without adding too much complexity
and in ways that still fit in a small 8-bit microcontroller?
Perhaps by somehow using those 4 bits wasted
every time we need to output a plaintext byte
that isn't already in the table?

*/

/*

FIXME:
experiment with
various fast-start techniques:

use a single context
(say, context 0)
for the first 128 words;
then copy those 128 words
to all num_contexts
and then use context-specific compression from them on.

Fill with 2 or more next-letter bytes initially,
then slow down to only 1 next-letter later.

*/

/*
FIXME:

Experiment with 3 variants:
(1) isprint() compressed bytes always represent themselves
byte compression
(2) compressed text contains only isprint() characters
printable compression
"human-readable compression"
(3) FUTURE: high-compression hybrid without those limits.
(using the same LZW pruning techniques as
printable compression, but allowing
compressed text to contain any byte).


(1)

Relatively easy to decode, because each compressed byte
has some fixed pre-allocated meaning:

* In the compressed data, high-bit-clear normal 7-bit ASCII bytes represent themselves,
* In the compressed data, only high-bit-set bytes represent a word.
* all isprint() bytes in the compressed text always represent themselves.
* CR, NL, tab characters always represent themselves.
* The special 0x00 byte in the compressed text indicates end-of-file.
* All the high-bit-set bytes in the compressed text always represent a longer string, *or* in some cases represent a literal byte that is not one of the isprint() bytes.
** nybble-oriented: hi-bit-set represents strings of nybbles (at least 3)
** byte-oriented: hi-bit-set represents strings of bytes (at least 2 bytes)
* The special escape bytes 0x10...0x1f are used when the plaintext needs a byte that is not one of the isprint() bytes. (Perhaps they always come in pairs).
* The 0x00..0x0f bytes never occur in the compressed text (except for CR, NL, tab representing themselves).
So "uncompressing" normal ASCII text documents
(containing only isprint(), CR, NL, tab characters)
results in exactly the same document.

Byte-oriented dictionary:

If the ".prefix_word_index" is 0x00,
then the entry represents exactly the one-byte string
stored in "last_letter".
(This seems to be required in order for the decompressor
to
decode a compressed byte
into a string of plaintext bytes that begins with
a plaintext byte that has the hi bit set,
or a plaintext byte that looks like any of the other special escape bytes).

If the ".prefix_word_index" has the high bit set
(i.e., is 0x80 or more),
then the entry represents
the concatenation of
the string represented by that previous index
(stored in dictionary(prefix_word_index - 80) )
and the last_letter.

Special case
(apparently not completely necessary,
but it seemed helpful at the time):
If the ".prefix_word_index" has the high bit clear
(but is not the special case 0x00),
(i.e., is in the range 0x01..0x7f),
then that previous word is a literal one-byte string
(not a reference to some other entry in the table),
and this entry represents exactly the two-byte string
the concatenation of
that previous byte and the last_letter.



Goal:
compressed text for 8-bit processors
using extremely little program space
and relatively little RAM.
(Each context stores only 128 entries, one for each hi-bit-set byte).
(Perhaps also format as a small "signature program").

(2)

Most difficult to decode, because each compressed character could represent either itself or some longer string,
which can change rapidly during decoding:
* Compressed text contains only isprint() characters.
So the original plaintext can be recovered from
characters printed on paper.
(Perhaps any amount of whitespace from the OCR'ed text is assumed to be a single space?)
(Perhaps decoder ignores whitespace entirely,
only the isgraph() characters are decoded?)
* At any one time during decompression,
the next character in the compressed text may represent:
* a character (try to make it represent itself)
* a stand-alone index character representing some other string of nybbles
** (initially, most bytes decode to a string of exactly 2 nybbles
that represent exactly the same byte;
later in the compressed text, some bytes decode to a much longer string,
perhaps things like " the ").
* (optional) a partial index character, followed by *any* character,
both characters together representing a string of nybbles.
* A few special characters are used as escapes
to represent single characters in the plaintext
when those characters cannot be represented in the plaintext
in any other way.
* (optional) a few special characters are used to escape larger blocks of literal plaintext, perhaps
"next character is literal",
"next 2 characters are literal", and
"the previous hexadecimal number indicates
how many of the following characters are literal".

The plaintext is considered an arbitrary stream of *nybbles*.
Does it
give better compression to
(a) assume it is byte-aligned,
and (for example) use the previous full aligned byte as the context, or
(b) use the previous 2 nybbles as context, whether or not they are aligned?

Because we have so few characters available,
use agressive pruning techniques to remove "words" from the dictionary
that we don't expect to see soon
and replace them with "words" that we do expect to see soon.
Frequently re-assigning the meaning of each character
dynamically during decompression.

(perhaps somehow pick characters
that are rarely used in the next part of *this* document
to use as escape characters,
something like the escapes in pucrunch?)

Perhaps try to make this *extremely* simple to decode by hand,
as a proposal for hand-written compression?
Perhaps only put integer numbers of characters
into each dictionary entry,
so this can be encoded and decoded by hand
with *any* available set of characters,
without requiring memorization of the ASCII characters ...
and perhaps exploiting a few common hand-written idioms
that are unavailable in ASCII,
such as circled character,
circled letter pairs,
extra dots over vowels,
extra slashes through characters
(reminiscent of "slashed 7", "slashed z",
single- and double-slashed dollar sign,
double-barred euro symbol and other currency symbols, etc.),
underlined words, etc.

Nybble-oriented dictionary:
16 special indexes -- bytes in the compressed text -- represent 1 nybble;
all other indexes, through their ".prefix_word_index" chain,
directly or indirectly lead to one of those 16 special indexes.

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

FUTURE:
experiment with "quine-friendly" compression
so that the compressed text,
other than a short teleomere,
contains only the characters
that represent themselves in a C string --
i.e., the isprint() characters, minus quote, minus backslash.
(Perhaps only base64url characters for simplicity?)
(Perhaps only characters used in Z85, the ZeroMQ base-85 encoding algorithm?)

(3)

A combination of (1) and (2),
agressively pruning and re-using *every* available byte,
with the goal of higher compression ratio.

Initially assign each plaintext bytes to represent itself,
(isprint, CR, NL, tab)
so the first 128 new "words" stored in the dictionary
during decompression
are represented by hi-bit-set bytes,
so initially it acts just like (1).
The next 32 new "words" in the dictionary
are represented by bytes 0x00..0x1f
(perhaps reserving 0x00 as end-of-compressed-text),
so it still acts much like (1)
except that CR, NL, tab characters are represented differently.
Then later isprint() characters that are rarely used in this text
are pruned and reassigned to more common strings.

When compressing plaintext that happens
to only contain the characters that (1) hardwires,
approach (3) runs the risk of occasionally
pruning one of those characters and using it to represent
a longer string,
and then later if we ever *need* that exact literal character,
we need to somehow "expand" it in the compressed text
with some sort of escape code.
Approach (1) guarantees that we can always
represent that exact character with, in the worst case,
one byte of compressed text.
However, perhaps some symbols,
are so extremely rare in ASCII text that it's worth it
to occasionally use 2 or 3 bytes for that symbol
when we really need it,
so we can free up that symbol to represent some other
much more common letter-pair
in a single byte.
Perhaps indicate in the header of the file
(the header of each block in the file?)
whether
(a) pre-fill the dictionary so every byte initially represents itself
(possibly good for binary files)
(b) pre-fill the dictionary so
a normal ASCII text byte initially represents itself,
and a bunch of (letter)(space) and (space)(letter) pairs
can be represented by one compressed byte
(good for European text).
(c) we use type (1) compression;
a normal ASCII text byte *always* represents itself.
(d) some other dictionary-fill technique.


Goal:
compressed text for 8-bit processors
with somewhat better compression than (1),
even if it requires slightly more program space and RAM.

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
}

bool
isprintable( const char * s ){
    while(*s){
        if( !isprint( *s ) ){
            return false;
        };
        s++;
    };
    return true;
}

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
}

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
}

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
}

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
}

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

    int compression_table[num_contexts][word_indexes][16] = {0};

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
    // int compression_table[num_contexts][word_indexes][16],
    int next_word_index[num_contexts],
    int context, const char * source, char * dest
    // , bool original_nybble_offset
){
    *dest = *source;
    increment_table_index( context, next_word_index );
    return 2;
}

void
test_byte_compress( const char * source_original, char * dest_original ){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;

    // int compression_table[num_contexts][word_indexes][16] = {0};

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
                // compression_table,
                next_word_index,
                context, source, dest
                // , nybble_offset
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
    // int next_word_index[num_contexts],
    // int context,
    const char * source, char * dest, bool original_nybble_offset
){
    int nybble_offset = (unsigned) original_nybble_offset;
    *dest = get_nybble( source, nybble_offset );
    return 1;
}


void test_nybble_compress( const char * source_original, char * dest_original ){
    const char * source = source_original;
    char * dest = dest_original;
    int compression_type = EIGHT_BIT_PRUNED;
    // int next_word_index[num_contexts] = {0};

    *dest++ = compression_type;
    /* FIXME: implement */
        source = source_original;
        // first byte copied unchanged, in order to provide context
        *dest++ = *source++;
        bool nybble_offset = 0;
        while( *source ){ // assume null-terminated string -- is this wise?
            int nybbles = test_nybble_compress_index(
                // next_word_index,
                // 0,
                source, dest, nybble_offset
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

