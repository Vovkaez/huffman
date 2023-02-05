# Archiving tool

A fast command line file archiving tool. Compression is performed via Huffman coding with 8-bit length codeword.

## Usage
The tool runs from console, here are the flags and parameters you can provide:

* `--compress` and `--decompress` to specify mode
* `--input <input-file>` to provide input file name
* `--output <output-file>` to provide output file name
* `-h`, `--help` to get information about usage

## Implementation details
Internally, input binary data is getting encoded with Huffman coding with 8-bit length codeword. Specifically, [canonical Huffman coding](https://en.wikipedia.org/wiki/Canonical_Huffman_code) is used for decoding and storing efficiency. Algorithm baseline is inspired by Mike Liddell and Alistair Moffat [research](http://www.ece.iit.edu/~biitcomm/research/Variable-Length%20Codes/prefix%20codes%20decoding/Decoding%20prefix%20codes.pdf) (page 1695, 1697).

## Output format
Output file format is as follows: array of 256 codeword lengths, each length encoded by 1 byte, then 1 byte to store the number of unused bits in the end of file, and the rest is the encoded message body.
