/************************************************************************************\

  hash.c - ATmega32U4 8-bit hash algorithm

  (c) 2008-2020 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com
 
  THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
  WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
  TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
  IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
  CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

\************************************************************************************/

#include "usb.h"

/************************************************************************************\
  Python script for generating 256 byte lookup table (T):

  from random import shuffle
  example_table = list(range(0, 256))
  shuffle(example_table)

\************************************************************************************/

static const uint8_t T[256] PROGMEM = { 190, 179, 236, 108, 4, 207, 5, 27, 21, 81, 189, 8, 132, 93, 123, 142, 58, 210, 165, 227,
 29, 72, 136, 2, 98, 64, 84, 85, 186, 113, 28, 128, 181, 75, 141, 56, 70, 112, 196, 3, 37, 231, 135, 86, 91, 54, 23, 41, 
 204, 225, 245, 109, 240, 164, 233, 211, 44, 25, 228, 48, 146, 7, 63, 102, 18, 234, 167, 40, 122, 176, 114, 76, 55, 6, 201,
 212, 39, 209, 59, 101, 173, 138, 10, 229, 226, 168, 99, 215, 17, 110, 124, 140, 169, 214, 221, 206, 42, 106, 87, 166, 107, 
 100, 202, 133, 249, 253, 134, 46, 178, 162, 126, 26, 243, 49, 78, 172, 235, 33, 199, 95, 252, 149, 188, 120, 145, 184, 152, 
 254, 88, 239, 194, 161, 96, 82, 79, 69, 143, 34, 1, 230, 155, 104, 242, 150, 139, 60, 182, 219, 222, 195, 20, 71, 208, 130, 
 13, 111, 170, 43, 205, 116, 103, 185, 45, 232, 19, 97, 129, 50, 197, 35, 94, 9, 32, 65, 61, 90, 125, 180, 177, 66, 47, 73, 
 77, 24, 220, 223, 148, 68, 0, 248, 11, 246, 137, 118, 121, 12, 218, 22, 159, 163, 131, 247, 183, 14, 157, 224, 147, 174, 
 193, 192, 74, 160, 52, 15, 237, 213, 89, 241, 200, 216, 191, 119, 251, 151, 30, 158, 144, 105, 53, 171, 250, 156, 175, 92, 
 115, 31, 255, 187, 203, 83, 38, 67, 127, 57, 198, 238, 217, 153, 117, 16, 36, 154, 80, 51, 62, 244 };

// Pearson hashing algorithm as described in Wikipedia. Equal to phash_original (more obfuscated).
// -> https://en.wikipedia.org/wiki/Pearson_hashing
// This assumes that key always ends with NULL character (\0).
uint8_t hash8(uint8_t *key) 
{
    uint8_t hash = 0;
    for (uint8_t c = *key++; c; c = *key++)
        hash = pgm_read_byte_near(T + (hash ^ c));
    return hash;
}
