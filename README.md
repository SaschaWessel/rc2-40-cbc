rc2-40-cbc
==========

A brute force tool for 40 Bit RC2 encrypted email messages.

S/MIME
------

In S/MIME messages [1] the plaintext is encrypted with a symmetric key
(e.g. 3DES, RC2) and this symmetric key is encrypted with an asymmetric
public key (e.g. RSA). To decrypt the message, only the symmetric key is
required and this key can be brute forced for small key sizes. This tool
implements this for 40 Bit RC2 keys.

[1] S/MIME Version 2 Message Specification: <http://tools.ietf.org/html/rfc2311>

RC2 and cipher-block chaining (CBC)
-----------------------------------

RC2 is a block cipher, which means that always one plaintext block (8 bytes) is
encrypted. To get different ciphertexts for identical plaintext blocks, each
block of plaintext is XORed with the previous ciphertext block before being
encrypted (CBC) [2]. For the first block an initialization vector (8 bytes) is
XORed with the plaintext block. The initialization vector is part of the S/MIME
message.

[2] Cipher-block chaining: <http://en.wikipedia.org/wiki/Cipher_block_chaining>

Brute force
-----------

The tool decrypts [3] a given 32 byte ciphertext with each possible key (2^40).
This will take around 150 - 250 CPU hours, which is around one day on a CPU
with 8 cores. If the plaintext only contains space and printable characters,
the key and the plaintext is written to stdout. Example:

    $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA
        IV: fedcba9876543210
    Cipher: 8ac497d81b21050df0e4b4d5ba39dc0c3e8af82b73ffc14038e465bcc37b0bda
     Range: 0000000000 - ffffffffff
    Status: 0001000000    0:00:11    0.002%  1525201/s  -> ETA 200:14:45
       Key: 0001020304  446174653a204d6f6e2c203136204d617920323031312030383a32313a333820  |Date: Mon, 16 May 2011 08:21:38 |

[3] RC2 source: <http://groups.google.com/group/sci.crypt/msg/f383f5dae68ebc70>

Usage
-----

1.  Install mpack, dumpasn1 and openssl:

        $ sudo apt-get install mpack dumpasn1 openssl

2.  Compile the code:

        $ make
        cc -Wall -g -O3 -funroll-loops -fwhole-program  -o rc2-40-cbc rc2-40-cbc.c

3.  Save the email message to a file and extract the S/MIME attachment:

        $ munpack example.mime
        smime.p7m (application/x-pkcs7-mime)

4.  Find the initialization vector (iv) and the ciphertext:

        $ dumpasn1 smime.p7m | grep rc2 -A 8
         452    8:           OBJECT IDENTIFIER rc2CBC (1 2 840 113549 3 2)
         462   14:           SEQUENCE {
         464    2:             INTEGER 160
         468    8:             OCTET STRING FE DC BA 98 76 54 32 10
                 :             }
                 :           }
         478 131408:         [0]
                 :           8A C4 97 D8 1B 21 05 0D F0 E4 B4 D5 BA 39 DC 0C
                 :           3E 8A F8 2B 73 FF C1 40 38 E4 65 BC C3 7B 0B DA
        0 warnings, 0 errors.

    Explanations:

        offset 464: rc2ParameterVersion (160 ->  40 effective-key-bits,
                                         120 ->  64 effective-key-bits,
                                          58 -> 128 effective-key-bits)
        offset 468: iv (8 bytes initialization vector)
        offset 478: ciphertext (131408 bytes, we need only the first 32 bytes)

5.  Brute force (may take some time, around 200 CPU hours in the example output below):

        $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA | tee log
            IV: fedcba9876543210
        Cipher: 8ac497d81b21050df0e4b4d5ba39dc0c3e8af82b73ffc14038e465bcc37b0bda
         Range: 0000000000 - ffffffffff
        Status: 0001000000    0:00:11    0.002%  1525201/s  -> ETA 200:14:45
           Key: 0001020304  446174653a204d6f6e2c203136204d617920323031312030383a32313a333820  |Date: Mon, 16 May 2011 08:21:38 |

    Limit the key to parallelize the calculation, e.g.:

        $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA 0000000000 3FFFFFFFFF
        $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA 4000000000 7FFFFFFFFF
        $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA 8000000000 BFFFFFFFFF
        $ ./rc2-40-cbc FEDCBA9876543210 8AC497D81B21050DF0E4B4D5BA39DC0C3E8AF82B73FFC14038E465BCC37B0BDA C000000000 FFFFFFFFFF

    NOTE: The tool searches for printable strings (isprint||isspace), so you might get false positives.

6.  Copy the message data (see dumpasn1 output):

        $ dd if=smime.p7m bs=1 skip=483 count=131408 of=smime.data

7.  Verify that you have copied the desired data (again, see dumpasn1 output):

        $ hexdump -C smime.data | head -n 1
        00000000  8a c4 97 d8 1b 21 05 0d  f0 e4 b4 d5 ba 39 dc 0c  |.....!.......9..|

8.  Decrypt the message:

        $ openssl rc2-40-cbc -d -in smime.data -out plain.mime -iv FEDCBA9876543210 -K 0001020304

9.  Unpack the MIME message:

        $ munpack -t plain.mime

Error messages
--------------

Gpgsm doesn't support the RC2 algorithm. Therefore you might have got one of
the following error messages due to an unsupported algorithm.

* mutt

        [-- Error: decryption failed: Unsupported algorithm --]

* gpgsm

        $ gpgsm -d smime.p7m
        gpgsm: unsupported algorithm `1.2.840.113549.3.2'
        gpgsm: (this is the RC2 algorithm)
        gpgsm: message decryption failed: Unsupported algorithm <GpgSM>

