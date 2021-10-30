
/* n_ary_huffman.c
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

*/

