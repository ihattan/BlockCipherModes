#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>

#define KEY_SIZE   16 // key size in bytes
#define STATE_SIZE 16 // state size in bytes
#define ROUNDS     11 // 11 rounds correlate to the 16-byte (128-bit) key
#define ENCRYPT    0
#define DECRYPT    1

/**
 * All lookup tables from https://cryptography.fandom.com/wiki/Rijndael_mix_columns
*/

static const unsigned char s[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

static const unsigned char inv_s[256] = {
	0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
	0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
	0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
	0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
	0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
	0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
	0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
	0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
	0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
	0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
	0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
	0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
	0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
	0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
	0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
	0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};

static const unsigned char rcon[255] = { 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36 };

static const unsigned char galois_2[256] = {
	0x00,0x02,0x04,0x06,0x08,0x0a,0x0c,0x0e,0x10,0x12,0x14,0x16,0x18,0x1a,0x1c,0x1e,
	0x20,0x22,0x24,0x26,0x28,0x2a,0x2c,0x2e,0x30,0x32,0x34,0x36,0x38,0x3a,0x3c,0x3e,
	0x40,0x42,0x44,0x46,0x48,0x4a,0x4c,0x4e,0x50,0x52,0x54,0x56,0x58,0x5a,0x5c,0x5e,
	0x60,0x62,0x64,0x66,0x68,0x6a,0x6c,0x6e,0x70,0x72,0x74,0x76,0x78,0x7a,0x7c,0x7e,
	0x80,0x82,0x84,0x86,0x88,0x8a,0x8c,0x8e,0x90,0x92,0x94,0x96,0x98,0x9a,0x9c,0x9e,
	0xa0,0xa2,0xa4,0xa6,0xa8,0xaa,0xac,0xae,0xb0,0xb2,0xb4,0xb6,0xb8,0xba,0xbc,0xbe,
	0xc0,0xc2,0xc4,0xc6,0xc8,0xca,0xcc,0xce,0xd0,0xd2,0xd4,0xd6,0xd8,0xda,0xdc,0xde,
	0xe0,0xe2,0xe4,0xe6,0xe8,0xea,0xec,0xee,0xf0,0xf2,0xf4,0xf6,0xf8,0xfa,0xfc,0xfe,
	0x1b,0x19,0x1f,0x1d,0x13,0x11,0x17,0x15,0x0b,0x09,0x0f,0x0d,0x03,0x01,0x07,0x05,
	0x3b,0x39,0x3f,0x3d,0x33,0x31,0x37,0x35,0x2b,0x29,0x2f,0x2d,0x23,0x21,0x27,0x25,
	0x5b,0x59,0x5f,0x5d,0x53,0x51,0x57,0x55,0x4b,0x49,0x4f,0x4d,0x43,0x41,0x47,0x45,
	0x7b,0x79,0x7f,0x7d,0x73,0x71,0x77,0x75,0x6b,0x69,0x6f,0x6d,0x63,0x61,0x67,0x65,
	0x9b,0x99,0x9f,0x9d,0x93,0x91,0x97,0x95,0x8b,0x89,0x8f,0x8d,0x83,0x81,0x87,0x85,
	0xbb,0xb9,0xbf,0xbd,0xb3,0xb1,0xb7,0xb5,0xab,0xa9,0xaf,0xad,0xa3,0xa1,0xa7,0xa5,
	0xdb,0xd9,0xdf,0xdd,0xd3,0xd1,0xd7,0xd5,0xcb,0xc9,0xcf,0xcd,0xc3,0xc1,0xc7,0xc5,
	0xfb,0xf9,0xff,0xfd,0xf3,0xf1,0xf7,0xf5,0xeb,0xe9,0xef,0xed,0xe3,0xe1,0xe7,0xe5};

static const unsigned char galois_3[256] = {
	0x00,0x03,0x06,0x05,0x0c,0x0f,0x0a,0x09,0x18,0x1b,0x1e,0x1d,0x14,0x17,0x12,0x11,
	0x30,0x33,0x36,0x35,0x3c,0x3f,0x3a,0x39,0x28,0x2b,0x2e,0x2d,0x24,0x27,0x22,0x21,
	0x60,0x63,0x66,0x65,0x6c,0x6f,0x6a,0x69,0x78,0x7b,0x7e,0x7d,0x74,0x77,0x72,0x71,
	0x50,0x53,0x56,0x55,0x5c,0x5f,0x5a,0x59,0x48,0x4b,0x4e,0x4d,0x44,0x47,0x42,0x41,
	0xc0,0xc3,0xc6,0xc5,0xcc,0xcf,0xca,0xc9,0xd8,0xdb,0xde,0xdd,0xd4,0xd7,0xd2,0xd1,
	0xf0,0xf3,0xf6,0xf5,0xfc,0xff,0xfa,0xf9,0xe8,0xeb,0xee,0xed,0xe4,0xe7,0xe2,0xe1,
	0xa0,0xa3,0xa6,0xa5,0xac,0xaf,0xaa,0xa9,0xb8,0xbb,0xbe,0xbd,0xb4,0xb7,0xb2,0xb1,
	0x90,0x93,0x96,0x95,0x9c,0x9f,0x9a,0x99,0x88,0x8b,0x8e,0x8d,0x84,0x87,0x82,0x81,
	0x9b,0x98,0x9d,0x9e,0x97,0x94,0x91,0x92,0x83,0x80,0x85,0x86,0x8f,0x8c,0x89,0x8a,
	0xab,0xa8,0xad,0xae,0xa7,0xa4,0xa1,0xa2,0xb3,0xb0,0xb5,0xb6,0xbf,0xbc,0xb9,0xba,
	0xfb,0xf8,0xfd,0xfe,0xf7,0xf4,0xf1,0xf2,0xe3,0xe0,0xe5,0xe6,0xef,0xec,0xe9,0xea,
	0xcb,0xc8,0xcd,0xce,0xc7,0xc4,0xc1,0xc2,0xd3,0xd0,0xd5,0xd6,0xdf,0xdc,0xd9,0xda,
	0x5b,0x58,0x5d,0x5e,0x57,0x54,0x51,0x52,0x43,0x40,0x45,0x46,0x4f,0x4c,0x49,0x4a,
	0x6b,0x68,0x6d,0x6e,0x67,0x64,0x61,0x62,0x73,0x70,0x75,0x76,0x7f,0x7c,0x79,0x7a,
	0x3b,0x38,0x3d,0x3e,0x37,0x34,0x31,0x32,0x23,0x20,0x25,0x26,0x2f,0x2c,0x29,0x2a,
	0x0b,0x08,0x0d,0x0e,0x07,0x04,0x01,0x02,0x13,0x10,0x15,0x16,0x1f,0x1c,0x19,0x1a};

static const unsigned char galois_9[256] = {
	0x00,0x09,0x12,0x1b,0x24,0x2d,0x36,0x3f,0x48,0x41,0x5a,0x53,0x6c,0x65,0x7e,0x77,
	0x90,0x99,0x82,0x8b,0xb4,0xbd,0xa6,0xaf,0xd8,0xd1,0xca,0xc3,0xfc,0xf5,0xee,0xe7,
	0x3b,0x32,0x29,0x20,0x1f,0x16,0x0d,0x04,0x73,0x7a,0x61,0x68,0x57,0x5e,0x45,0x4c,
	0xab,0xa2,0xb9,0xb0,0x8f,0x86,0x9d,0x94,0xe3,0xea,0xf1,0xf8,0xc7,0xce,0xd5,0xdc,
	0x76,0x7f,0x64,0x6d,0x52,0x5b,0x40,0x49,0x3e,0x37,0x2c,0x25,0x1a,0x13,0x08,0x01,
	0xe6,0xef,0xf4,0xfd,0xc2,0xcb,0xd0,0xd9,0xae,0xa7,0xbc,0xb5,0x8a,0x83,0x98,0x91,
	0x4d,0x44,0x5f,0x56,0x69,0x60,0x7b,0x72,0x05,0x0c,0x17,0x1e,0x21,0x28,0x33,0x3a,
	0xdd,0xd4,0xcf,0xc6,0xf9,0xf0,0xeb,0xe2,0x95,0x9c,0x87,0x8e,0xb1,0xb8,0xa3,0xaa,
	0xec,0xe5,0xfe,0xf7,0xc8,0xc1,0xda,0xd3,0xa4,0xad,0xb6,0xbf,0x80,0x89,0x92,0x9b,
	0x7c,0x75,0x6e,0x67,0x58,0x51,0x4a,0x43,0x34,0x3d,0x26,0x2f,0x10,0x19,0x02,0x0b,
	0xd7,0xde,0xc5,0xcc,0xf3,0xfa,0xe1,0xe8,0x9f,0x96,0x8d,0x84,0xbb,0xb2,0xa9,0xa0,
	0x47,0x4e,0x55,0x5c,0x63,0x6a,0x71,0x78,0x0f,0x06,0x1d,0x14,0x2b,0x22,0x39,0x30,
	0x9a,0x93,0x88,0x81,0xbe,0xb7,0xac,0xa5,0xd2,0xdb,0xc0,0xc9,0xf6,0xff,0xe4,0xed,
	0x0a,0x03,0x18,0x11,0x2e,0x27,0x3c,0x35,0x42,0x4b,0x50,0x59,0x66,0x6f,0x74,0x7d,
	0xa1,0xa8,0xb3,0xba,0x85,0x8c,0x97,0x9e,0xe9,0xe0,0xfb,0xf2,0xcd,0xc4,0xdf,0xd6,
	0x31,0x38,0x23,0x2a,0x15,0x1c,0x07,0x0e,0x79,0x70,0x6b,0x62,0x5d,0x54,0x4f,0x46};

static const unsigned char galois_b[256] = {
	0x00,0x0b,0x16,0x1d,0x2c,0x27,0x3a,0x31,0x58,0x53,0x4e,0x45,0x74,0x7f,0x62,0x69,
	0xb0,0xbb,0xa6,0xad,0x9c,0x97,0x8a,0x81,0xe8,0xe3,0xfe,0xf5,0xc4,0xcf,0xd2,0xd9,
	0x7b,0x70,0x6d,0x66,0x57,0x5c,0x41,0x4a,0x23,0x28,0x35,0x3e,0x0f,0x04,0x19,0x12,
	0xcb,0xc0,0xdd,0xd6,0xe7,0xec,0xf1,0xfa,0x93,0x98,0x85,0x8e,0xbf,0xb4,0xa9,0xa2,
	0xf6,0xfd,0xe0,0xeb,0xda,0xd1,0xcc,0xc7,0xae,0xa5,0xb8,0xb3,0x82,0x89,0x94,0x9f,
	0x46,0x4d,0x50,0x5b,0x6a,0x61,0x7c,0x77,0x1e,0x15,0x08,0x03,0x32,0x39,0x24,0x2f,
	0x8d,0x86,0x9b,0x90,0xa1,0xaa,0xb7,0xbc,0xd5,0xde,0xc3,0xc8,0xf9,0xf2,0xef,0xe4,
	0x3d,0x36,0x2b,0x20,0x11,0x1a,0x07,0x0c,0x65,0x6e,0x73,0x78,0x49,0x42,0x5f,0x54,
	0xf7,0xfc,0xe1,0xea,0xdb,0xd0,0xcd,0xc6,0xaf,0xa4,0xb9,0xb2,0x83,0x88,0x95,0x9e,
	0x47,0x4c,0x51,0x5a,0x6b,0x60,0x7d,0x76,0x1f,0x14,0x09,0x02,0x33,0x38,0x25,0x2e,
	0x8c,0x87,0x9a,0x91,0xa0,0xab,0xb6,0xbd,0xd4,0xdf,0xc2,0xc9,0xf8,0xf3,0xee,0xe5,
	0x3c,0x37,0x2a,0x21,0x10,0x1b,0x06,0x0d,0x64,0x6f,0x72,0x79,0x48,0x43,0x5e,0x55,
	0x01,0x0a,0x17,0x1c,0x2d,0x26,0x3b,0x30,0x59,0x52,0x4f,0x44,0x75,0x7e,0x63,0x68,
	0xb1,0xba,0xa7,0xac,0x9d,0x96,0x8b,0x80,0xe9,0xe2,0xff,0xf4,0xc5,0xce,0xd3,0xd8,
	0x7a,0x71,0x6c,0x67,0x56,0x5d,0x40,0x4b,0x22,0x29,0x34,0x3f,0x0e,0x05,0x18,0x13,
	0xca,0xc1,0xdc,0xd7,0xe6,0xed,0xf0,0xfb,0x92,0x99,0x84,0x8f,0xbe,0xb5,0xa8,0xa3};

static const unsigned char galois_d[256] = {
	0x00,0x0d,0x1a,0x17,0x34,0x39,0x2e,0x23,0x68,0x65,0x72,0x7f,0x5c,0x51,0x46,0x4b,
	0xd0,0xdd,0xca,0xc7,0xe4,0xe9,0xfe,0xf3,0xb8,0xb5,0xa2,0xaf,0x8c,0x81,0x96,0x9b,
	0xbb,0xb6,0xa1,0xac,0x8f,0x82,0x95,0x98,0xd3,0xde,0xc9,0xc4,0xe7,0xea,0xfd,0xf0,
	0x6b,0x66,0x71,0x7c,0x5f,0x52,0x45,0x48,0x03,0x0e,0x19,0x14,0x37,0x3a,0x2d,0x20,
	0x6d,0x60,0x77,0x7a,0x59,0x54,0x43,0x4e,0x05,0x08,0x1f,0x12,0x31,0x3c,0x2b,0x26,
	0xbd,0xb0,0xa7,0xaa,0x89,0x84,0x93,0x9e,0xd5,0xd8,0xcf,0xc2,0xe1,0xec,0xfb,0xf6,
	0xd6,0xdb,0xcc,0xc1,0xe2,0xef,0xf8,0xf5,0xbe,0xb3,0xa4,0xa9,0x8a,0x87,0x90,0x9d,
	0x06,0x0b,0x1c,0x11,0x32,0x3f,0x28,0x25,0x6e,0x63,0x74,0x79,0x5a,0x57,0x40,0x4d,
	0xda,0xd7,0xc0,0xcd,0xee,0xe3,0xf4,0xf9,0xb2,0xbf,0xa8,0xa5,0x86,0x8b,0x9c,0x91,
	0x0a,0x07,0x10,0x1d,0x3e,0x33,0x24,0x29,0x62,0x6f,0x78,0x75,0x56,0x5b,0x4c,0x41,
	0x61,0x6c,0x7b,0x76,0x55,0x58,0x4f,0x42,0x09,0x04,0x13,0x1e,0x3d,0x30,0x27,0x2a,
	0xb1,0xbc,0xab,0xa6,0x85,0x88,0x9f,0x92,0xd9,0xd4,0xc3,0xce,0xed,0xe0,0xf7,0xfa,
	0xb7,0xba,0xad,0xa0,0x83,0x8e,0x99,0x94,0xdf,0xd2,0xc5,0xc8,0xeb,0xe6,0xf1,0xfc,
	0x67,0x6a,0x7d,0x70,0x53,0x5e,0x49,0x44,0x0f,0x02,0x15,0x18,0x3b,0x36,0x21,0x2c,
	0x0c,0x01,0x16,0x1b,0x38,0x35,0x22,0x2f,0x64,0x69,0x7e,0x73,0x50,0x5d,0x4a,0x47,
	0xdc,0xd1,0xc6,0xcb,0xe8,0xe5,0xf2,0xff,0xb4,0xb9,0xae,0xa3,0x80,0x8d,0x9a,0x97};

static const unsigned char galois_e[256] = {
	0x00,0x0e,0x1c,0x12,0x38,0x36,0x24,0x2a,0x70,0x7e,0x6c,0x62,0x48,0x46,0x54,0x5a,
	0xe0,0xee,0xfc,0xf2,0xd8,0xd6,0xc4,0xca,0x90,0x9e,0x8c,0x82,0xa8,0xa6,0xb4,0xba,
	0xdb,0xd5,0xc7,0xc9,0xe3,0xed,0xff,0xf1,0xab,0xa5,0xb7,0xb9,0x93,0x9d,0x8f,0x81,
	0x3b,0x35,0x27,0x29,0x03,0x0d,0x1f,0x11,0x4b,0x45,0x57,0x59,0x73,0x7d,0x6f,0x61,
	0xad,0xa3,0xb1,0xbf,0x95,0x9b,0x89,0x87,0xdd,0xd3,0xc1,0xcf,0xe5,0xeb,0xf9,0xf7,
	0x4d,0x43,0x51,0x5f,0x75,0x7b,0x69,0x67,0x3d,0x33,0x21,0x2f,0x05,0x0b,0x19,0x17,
	0x76,0x78,0x6a,0x64,0x4e,0x40,0x52,0x5c,0x06,0x08,0x1a,0x14,0x3e,0x30,0x22,0x2c,
	0x96,0x98,0x8a,0x84,0xae,0xa0,0xb2,0xbc,0xe6,0xe8,0xfa,0xf4,0xde,0xd0,0xc2,0xcc,
	0x41,0x4f,0x5d,0x53,0x79,0x77,0x65,0x6b,0x31,0x3f,0x2d,0x23,0x09,0x07,0x15,0x1b,
	0xa1,0xaf,0xbd,0xb3,0x99,0x97,0x85,0x8b,0xd1,0xdf,0xcd,0xc3,0xe9,0xe7,0xf5,0xfb,
	0x9a,0x94,0x86,0x88,0xa2,0xac,0xbe,0xb0,0xea,0xe4,0xf6,0xf8,0xd2,0xdc,0xce,0xc0,
	0x7a,0x74,0x66,0x68,0x42,0x4c,0x5e,0x50,0x0a,0x04,0x16,0x18,0x32,0x3c,0x2e,0x20,
	0xec,0xe2,0xf0,0xfe,0xd4,0xda,0xc8,0xc6,0x9c,0x92,0x80,0x8e,0xa4,0xaa,0xb8,0xb6,
	0x0c,0x02,0x10,0x1e,0x34,0x3a,0x28,0x26,0x7c,0x72,0x60,0x6e,0x44,0x4a,0x58,0x56,
	0x37,0x39,0x2b,0x25,0x0f,0x01,0x13,0x1d,0x47,0x49,0x5b,0x55,0x7f,0x71,0x63,0x6d,
	0xd7,0xd9,0xcb,0xc5,0xef,0xe1,0xf3,0xfd,0xa7,0xa9,0xbb,0xb5,0x9f,0x91,0x83,0x8d};

void pretty_print(unsigned char *in, int bytes) {
	for (int i = 0; i < bytes; i += 16) {
		printf("%02x %02x %02x %02x\n%02x %02x %02x %02x\n%02x %02x %02x %02x\n%02x %02x %02x %02x\n",
		in[i+0x0], in[i+0x1], in[i+0x2], in[i+0x3], in[i+0x4], in[i+0x5], in[i+0x6], in[i+0x7],
		in[i+0x8], in[i+0x9], in[i+0xa], in[i+0xb], in[i+0xc], in[i+0xd], in[i+0xe], in[i+0xf]);
	}
}

void pretty_print_decimal(unsigned char *in, int bytes) {
	for (int i = 0; i < bytes; i += 16) {
		printf("%03d %03d %03d %03d\n%03d %03d %03d %03d\n%03d %03d %03d %03d\n%03d %03d %03d %03d\n",
		in[i+0x0], in[i+0x1], in[i+0x2], in[i+0x3], in[i+0x4], in[i+0x5], in[i+0x6], in[i+0x7],
		in[i+0x8], in[i+0x9], in[i+0xa], in[i+0xb], in[i+0xc], in[i+0xd], in[i+0xe], in[i+0xf]);
	}
}

/**
 * XOR's word, a, with word, b, and stores it in res
**/
static void xor(unsigned char *a, unsigned char *b, unsigned char* res) {
	res[0] = a[0] ^ b[0];
	res[1] = a[1] ^ b[1];
	res[2] = a[2] ^ b[2];
	res[3] = a[3] ^ b[3];
}

static void g(unsigned char *in, unsigned char *res, int r) {
	// left circular shift
	res[0] = in[1];
	res[1] = in[2];
	res[2] = in[3];
	res[3] = in[0];

	// byte substition
	res[0] = s[((res[0]>>4)*16) + (res[0]&0xf)];
	res[1] = s[((res[1]>>4)*16) + (res[1]&0xf)];
	res[2] = s[((res[2]>>4)*16) + (res[2]&0xf)];
	res[3] = s[((res[3]>>4)*16) + (res[3]&0xf)];

	// xor with rcon
	res[0] ^= rcon[r];
}

static void generate_round_keys(unsigned char *key, unsigned char *round_keys) {
	int i = 0;  // i corresponds to the current word
				// floor(i/4) corresponds to current round

	/**
	 * copying the key into the first round
	**/

	// KEY_SIZE / 4 because there are 4 bytes per word
	while(i < KEY_SIZE / 4) {
		round_keys[(i*4)+0] = key[(i*4)+0];
		round_keys[(i*4)+1] = key[(i*4)+1];
		round_keys[(i*4)+2] = key[(i*4)+2];
		round_keys[(i*4)+3] = key[(i*4)+3];
		i += 1;
	}

	/**
	 * generating the next 10 rounds
	**/

	// current 4 words in rotation
	unsigned char w_0[4];
	unsigned char w_1[4];
	unsigned char w_2[4];
	unsigned char w_3[4];

	// next 4 words to generate
	unsigned char w_4[4];
	unsigned char w_5[4];
	unsigned char w_6[4];
	unsigned char w_7[4];

	// result of g()
	unsigned char g_r[4];

	// advance until all 11 rounds filled,
	// ROUNDS - 1 because we generate the NEXT round using the current round
	// KEY_SIZE/4 again because words, not bytes
	while (i <= (ROUNDS - 1) * KEY_SIZE/4) {
		// copy previous 4 words into current
		w_0[0] = round_keys[((i-4)*4)+0];
		w_1[0] = round_keys[((i-4)*4)+1];
		w_2[0] = round_keys[((i-4)*4)+2];
		w_3[0] = round_keys[((i-4)*4)+3];

		w_0[1] = round_keys[((i-3)*4)+0];
		w_1[1] = round_keys[((i-3)*4)+1];
		w_2[1] = round_keys[((i-3)*4)+2];
		w_3[1] = round_keys[((i-3)*4)+3];

		w_0[2] = round_keys[((i-2)*4)+0];
		w_1[2] = round_keys[((i-2)*4)+1];
		w_2[2] = round_keys[((i-2)*4)+2];
		w_3[2] = round_keys[((i-2)*4)+3];

		w_0[3] = round_keys[((i-1)*4)+0];
		w_1[3] = round_keys[((i-1)*4)+1];
		w_2[3] = round_keys[((i-1)*4)+2];
		w_3[3] = round_keys[((i-1)*4)+3];

		round_keys[((i+0)*4)+0] = w_0[0];
		round_keys[((i+0)*4)+1] = w_1[0];
		round_keys[((i+0)*4)+2] = w_2[0];
		round_keys[((i+0)*4)+3] = w_3[0];

		round_keys[((i+1)*4)+0] = w_0[1];
		round_keys[((i+1)*4)+1] = w_1[1];
		round_keys[((i+1)*4)+2] = w_2[1];
		round_keys[((i+1)*4)+3] = w_3[1];

		round_keys[((i+2)*4)+0] = w_0[2];
		round_keys[((i+2)*4)+1] = w_1[2];
		round_keys[((i+2)*4)+2] = w_2[2];
		round_keys[((i+2)*4)+3] = w_3[2];

		round_keys[((i+3)*4)+0] = w_0[3];
		round_keys[((i+3)*4)+1] = w_1[3];
		round_keys[((i+3)*4)+2] = w_2[3];
		round_keys[((i+3)*4)+3] = w_3[3];

		// (i/4) because g() requires the current round,
		// i corresponds to the current word, four words per round
		g(w_3, g_r, (i/4));

		xor(g_r, w_0, w_4);
		xor(w_4, w_1, w_5);
		xor(w_5, w_2, w_6);
		xor(w_6, w_3, w_7);

		round_keys[((i+0)*4)+0] = w_4[0];
		round_keys[((i+0)*4)+1] = w_5[0];
		round_keys[((i+0)*4)+2] = w_6[0];
		round_keys[((i+0)*4)+3] = w_7[0];

		round_keys[((i+1)*4)+0] = w_4[1];
		round_keys[((i+1)*4)+1] = w_5[1];
		round_keys[((i+1)*4)+2] = w_6[1];
		round_keys[((i+1)*4)+3] = w_7[1];

		round_keys[((i+2)*4)+0] = w_4[2];
		round_keys[((i+2)*4)+1] = w_5[2];
		round_keys[((i+2)*4)+2] = w_6[2];
		round_keys[((i+2)*4)+3] = w_7[2];

		round_keys[((i+3)*4)+0] = w_4[3];
		round_keys[((i+3)*4)+1] = w_5[3];
		round_keys[((i+3)*4)+2] = w_6[3];
		round_keys[((i+3)*4)+3] = w_7[3];

		i += 4;
	}

}

static void add_round_key(unsigned char *state, unsigned char *round_keys, int round) {

	int round_offset = round<<4;

	for (int i = 0; i < STATE_SIZE; i++) {
		state[i] ^= round_keys[round_offset + i];
	}
}

static void sub_bytes(unsigned char *state) {
	for (int i = 0; i < STATE_SIZE; i++) {
		state[i] = s[(state[i]&0xf0) + (state[i]&0xf)];
	}
}

static void inv_sub_bytes(unsigned char *state) {
	for (int i = 0; i < STATE_SIZE; i++) {
		state[i] = inv_s[(state[i]&0xf0) + (state[i]&0xf)];
	}
}

static void shift_rows(unsigned char *state) {
	unsigned char swap;

	swap = state[4];
	state[4] = state[5];
	state[5] = state[6];
	state[6] = state[7];
	state[7] = swap;

	swap = state[8];
	state[ 8] = state[10];
	state[10] = swap;
	swap = state[9];
	state[ 9] = state[11];
	state[11] = swap;

	swap = state[12];
	state[12] = state[15];
	state[15] = state[14];
	state[14] = state[13];
	state[13] = swap;
}

static void inv_shift_rows(unsigned char *state) {
	unsigned char swap;

	swap = state[4];
	state[4] = state[7];
	state[7] = state[6];
	state[6] = state[5];
	state[5] = swap;

	swap = state[8];
	state[ 8] = state[10];
	state[10] = swap;
	swap = state[9];
	state[ 9] = state[11];
	state[11] = swap;

	swap = state[12];
	state[12] = state[13];
	state[13] = state[14];
	state[14] = state[15];
	state[15] = swap;
}

// mix columns algorithm from https://cryptography.fandom.com/wiki/Rijndael_mix_columns
static void mix_columns(unsigned char *state) {
	unsigned char a[4];

	int current_column = 0;
	while (current_column < 4) { // 4 columns in the state
		a[0] = state[current_column +  0];
		a[1] = state[current_column +  4];
		a[2] = state[current_column +  8];
		a[3] = state[current_column + 12];

		state[current_column +  0] = galois_2[a[0]] ^ galois_3[a[1]] ^          a[2]  ^          a[3] ;
		state[current_column +  4] =          a[0]  ^ galois_2[a[1]] ^ galois_3[a[2]] ^          a[3] ;
		state[current_column +  8] =          a[0]  ^          a[1]  ^ galois_2[a[2]] ^ galois_3[a[3]];
		state[current_column + 12] = galois_3[a[0]] ^          a[1]  ^          a[2]  ^ galois_2[a[3]];

		current_column += 1;
	}
}

// inv_mix_columns algorithm from https://cryptography.fandom.com/wiki/Rijndael_mix_columns
static void inv_mix_columns(unsigned char *state) {
	unsigned char a[4];

	int current_column = 0;
	while (current_column < 4) { // 4 columns in the state
		a[0] = state[current_column +  0];
		a[1] = state[current_column +  4];
		a[2] = state[current_column +  8];
		a[3] = state[current_column + 12];

		state[current_column +  0] = galois_e[a[0]] ^ galois_b[a[1]] ^ galois_d[a[2]] ^ galois_9[a[3]];
		state[current_column +  4] = galois_9[a[0]] ^ galois_e[a[1]] ^ galois_b[a[2]] ^ galois_d[a[3]];
		state[current_column +  8] = galois_d[a[0]] ^ galois_9[a[1]] ^ galois_e[a[2]] ^ galois_b[a[3]];
		state[current_column + 12] = galois_b[a[0]] ^ galois_d[a[1]] ^ galois_9[a[2]] ^ galois_e[a[3]];

		current_column += 1;
	}
}

static void encrypt_block_cipher(unsigned char *key, unsigned char *input, unsigned char *state) {
	unsigned char round_keys[ROUNDS * KEY_SIZE];
	generate_round_keys(key, round_keys);

	// first state is input
	for (int i = 0; i < KEY_SIZE; i++) {
		state[i] = input[i];
	}

	int current_round = 0;

	// round 0, just add_round_key
	add_round_key(state, round_keys, current_round);
	current_round += 1;

	// last round is different
	while (current_round < ROUNDS - 1) {
			sub_bytes(state);
			shift_rows(state);
			mix_columns(state);
			add_round_key(state, round_keys, current_round);

			current_round += 1;
	}

	//final round
	sub_bytes(state);
	shift_rows(state);
	add_round_key(state, round_keys, current_round);
}

static void decrypt_block_cipher(unsigned char *key, unsigned char *input, unsigned char *state) {
	unsigned char round_keys[ROUNDS * KEY_SIZE];
	generate_round_keys(key, round_keys);

	// first state is input
	for (int i = 0; i < KEY_SIZE; i++) {
		state[i] = input[i];
	}

	int current_round = ROUNDS-1;

	// round 0 == add_round_key()
	add_round_key(state, round_keys, current_round);
	current_round -= 1;

	while (current_round > 0) {
		inv_shift_rows(state);
		inv_sub_bytes(state);
		add_round_key(state, round_keys, current_round);
		inv_mix_columns(state);

		current_round -= 1;
	}

	inv_shift_rows(state);
	inv_sub_bytes(state);
	add_round_key(state, round_keys, current_round);
}

static PyObject* blockcipher(PyObject* self, PyObject* args) {
	PyObject* key_list;
	PyObject* in_list;
	int mode_in;

	if (!PyArg_ParseTuple(args, "OOi", &key_list, &in_list, &mode_in)) {
		printf("fail\n");
		return NULL;
	}

	if (!PyList_CheckExact(key_list) || !PyList_CheckExact(in_list)) {
		printf("blockcipher handed something not a list\n");
		return NULL;
	}

	int in_size = (int)PyList_Size(in_list);

	if (in_size != STATE_SIZE) {
		printf("blockcipher handed list with invalid size\n");
		return NULL;
	}

	unsigned char key[KEY_SIZE];
	unsigned char input[STATE_SIZE];
	unsigned char output[STATE_SIZE];

	for (int i = 0; i < KEY_SIZE; i++) {
		key[i] = (unsigned char)PyLong_AsLong(PyList_GetItem(key_list, i));
	}

	for (int i = 0; i < STATE_SIZE; i++) {
		input[i] = (unsigned char)PyLong_AsLong(PyList_GetItem(in_list, i));
	}

	if (mode_in == ENCRYPT) {
		encrypt_block_cipher(key, input, output);
	} else if (mode_in == DECRYPT) {
		decrypt_block_cipher(key, input, output);
	} else {
		printf("INVALID ENCRYPT/DECRYPT MODE\n");
		return NULL;
	}

	PyObject* out_list = PyList_New(STATE_SIZE);
	for (int i = 0; i < STATE_SIZE; i++) {
		PyList_SetItem(out_list, i, PyLong_FromLong((long)output[i]));
	}

	return out_list;
}

static PyMethodDef blockcipher_funcs[] = {
	{ "blockcipher", (PyCFunction)blockcipher, METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef blockciphermodule = {
	PyModuleDef_HEAD_INIT,
	"blockcipher",
	NULL,
	-1,
	blockcipher_funcs
};

PyMODINIT_FUNC PyInit_blockcipher(void) {
	return PyModule_Create(&blockciphermodule);
}
