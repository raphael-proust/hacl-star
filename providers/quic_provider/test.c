#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "EverCrypt.h"
#include "quic_provider.h"

#ifndef CDECL
  #if _WIN32
    #define CDECL __cdecl
  #else
    #define CDECL
  #endif
#endif

void check_result(const char* testname, const unsigned char *actual, const unsigned char *expected, uint32_t length)
{
    for (uint32_t i=0; i<length; ++i) {
        if (actual[i] != expected[i]) {
            printf("FAIL %s:  actual 0x%2.2x mismatch with expected 0x%2.2x at offset %u\n",
              testname, actual[i], expected[i], i);
            exit(1);
        }
    }
}

void dump(const unsigned char buffer[], size_t len)
{
  size_t i;
  for(i=0; i<len; i++) {
    printf("%02x",buffer[i] & 0xFF);
    if (i % 32 == 31 || i == len-1) printf("\n");
  }
}

// Older coverage tests
void coverage(void)
{
  printf("==== coverage ====\n");

  unsigned char hash[64] = {0};
  unsigned char input[1] = {0};
  printf("SHA256('') =\n");
  quic_crypto_hash(TLS_hash_SHA256, hash, input, 0);
  dump(hash, 32);
  printf("\nSHA384('') = \n");
  quic_crypto_hash(TLS_hash_SHA384, hash, input, 0);
  dump(hash, 48);

//  printf("\nSHA512('') = \n");
//  quic_crypto_hash(TLS_hash_SHA512, hash, input, 0);
//  dump(hash, 64);

  unsigned char *key = (unsigned char *)"Jefe";
  unsigned char *data = (unsigned char *)"what do ya want for nothing?";

  printf("\nHMAC-SHA256('Jefe', 'what do ya want for nothing?') = \n");
  quic_crypto_hmac(TLS_hash_SHA256, hash, key, 4, data, 28);
  dump(hash, 32);
  assert(memcmp(hash, "\x5b\xdc\xc1\x46\xbf\x60\x75\x4e\x6a\x04\x24\x26\x08\x95\x75\xc7\x5a\x00\x3f\x08\x9d\x27\x39\x83\x9d\xec\x58\xb9\x64\xec\x38\x43", 32) == 0);

  printf("\nHMAC-SHA384('Jefe', 'what do ya want for nothing?') = \n");
  quic_crypto_hmac(TLS_hash_SHA384, hash, key, 4, data, 28);
  dump(hash, 48);
  assert(memcmp(hash, "\xaf\x45\xd2\xe3\x76\x48\x40\x31\x61\x7f\x78\xd2\xb5\x8a\x6b\x1b\x9c\x7e\xf4\x64\xf5\xa0\x1b\x47\xe4\x2e\xc3\x73\x63\x22\x44\x5e\x8e\x22\x40\xca\x5e\x69\xe2\xc7\x8b\x32\x39\xec\xfa\xb2\x16\x49", 48) == 0);

//  printf("\nHMAC-SHA512('Jefe', 'what do ya want for nothing?') = \n");
//  quic_crypto_hmac(TLS_hash_SHA512, hash, key, 4, data, 28);
//  dump(hash, 64);
//  assert(memcmp(hash, "\x16\x4b\x7a\x7b\xfc\xf8\x19\xe2\xe3\x95\xfb\xe7\x3b\x56\xe0\xa3\x87\xbd\x64\x22\x2e\x83\x1f\xd6\x10\x27\x0c\xd7\xea\x25\x05\x54\x97\x58\xbf\x75\xc0\x5a\x99\x4a\x6d\x03\x4f\x65\xf8\xf0\xe6\xfd\xca\xea\xb1\xa3\x4d\x4a\x6b\x4b\x63\x6e\x07\x0a\x38\xbc\xe7\x37", 64) == 0);

  unsigned char *salt = (unsigned char *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c";
  unsigned char *ikm = (unsigned char *)"\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b";
  unsigned char *info = (unsigned char *)"\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9";

  printf("\nprk = HKDF-EXTRACT-SHA256('0x000102030405060708090a0b0c', '0x0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b')\n");
  quic_crypto_hkdf_extract(TLS_hash_SHA256, hash, salt, 13, ikm, 22);
  dump(hash, 32);

  unsigned char prk[32] = {0};
  memcpy(prk, hash, 32);
  dump(prk, 32);

  unsigned char okm[42] = {0};
  printf("\nokm = HKDF-EXPAND-SHA256(prk, '0xf0f1f2f3f4f5f6f7f8f9', 42)\n");
  if(!quic_crypto_hkdf_expand(TLS_hash_SHA256, okm, 42, prk, 32, info, 10))
  {
    printf("Failed to call HKDF-expand\n");
    exit(1);
  }
  dump(okm, 42);

  quic_secret s = { .hash = TLS_hash_SHA256, .ae = TLS_aead_AES_128_GCM };
  memcpy(s.secret, hash, 32);
  quic_crypto_tls_derive_secret(&s, &s, "EXPORTER-QUIC server 1-RTT Secret");

  quic_key* k;
  if(!quic_crypto_derive_key(&k, &s))
  {
    printf("Failed to derive key\n");
    exit(1);
  }

  unsigned char cipher[128];
  printf("\nAES-128-GCM encrypt test:\n");
  quic_crypto_encrypt(k, cipher, 0, salt, 13, data, 28);
  dump(cipher, 28+16);
  quic_crypto_decrypt(k, hash, 0, salt, 13, cipher, 28+16);
  if(memcmp(hash, data, 28) != 0)
  {
    printf("Roundtrip decryption failed.\n");
    exit(1);
  }
  assert(quic_crypto_decrypt(k, hash, 1, salt, 13, cipher, 28+16) < 0);
  assert(quic_crypto_decrypt(k, hash, 0, salt, 12, cipher, 28+16) < 0);
  assert(quic_crypto_decrypt(k, hash, 0, salt, 13, cipher+1, 27+16) < 0);

  unsigned char *expected_pnmask = (unsigned char *)"\x16\x53\x7a\x9a";
  unsigned char pnmask[4];
  if(quic_crypto_packet_number_otp(k, cipher, pnmask))
  {
    printf("PN encryption mask:\n");
    dump(pnmask, 4);
    check_result("PN encryption mask", pnmask, expected_pnmask, sizeof(pnmask));
  } else {
    printf("PN encryption failed.\n");
    exit(1);
  }

  if(quic_crypto_decrypt(k, hash, 0, salt, 13, cipher, 28+16)) {
    printf("DECRYPT SUCCESS: \n");
    dump(hash, 28);
  } else {
    printf("DECRYPT FAILED.\n");
    exit(1);
  }

  quic_crypto_free_key(k);

  s.hash = TLS_hash_SHA256;
  s.ae = TLS_aead_CHACHA20_POLY1305;

  if(!quic_crypto_derive_key(&k, &s))
  {
    printf("Failed to derive key\n");
    exit(1);
  }

  printf("\nCHACHA20-POLY1305 encrypt test:\n");
  quic_crypto_encrypt(k, cipher, 0x29e255a7, salt, 13, data, 28);
  dump(cipher, 28+16);

  expected_pnmask = (unsigned char *)"\x5b\xe1\x8d\xf8";
  if(quic_crypto_packet_number_otp(k, cipher, pnmask))
  {
    printf("PN encryption mask:\n");
    dump(pnmask, 4);
    check_result("PN encryption mask", pnmask, expected_pnmask, sizeof(pnmask));
  } else {
    printf("PN encryption failed.\n");
    exit(1);
  }

  if(quic_crypto_decrypt(k, hash, 0x29e255a7, salt, 13, cipher, 28+16)) {
    printf("DECRYPT SUCCESS: \n");
    dump(hash, 28);
  } else {
    printf("DECRYPT FAILED.\n");
  }

  quic_crypto_free_key(k);
  printf("==== PASS:  coverage ====\n\n\n");
}

//////////////////////////////////////////////////////////////////////////////


const uint64_t sn = 0x29;

#define ad_len      0x11
const unsigned char ad[ad_len] =
{0xff,0x00,0x00,0x4f,0xee,0x00,0x00,0x42,0x37,0xff,0x00,0x00,0x09,0x00,0x00,0x00};

#define plain_len 256
const unsigned char plain[plain_len] = {
0x12,0x00,0x40,0xda,0x16,0x03,0x03,0x00,0xd5,0x01,0x00,0x00,0xd1,0x03,0x03,0x5a,
0xa0,0x4e,0xf1,0x3b,0x74,0x7c,0x34,0xe6,0x74,0x05,0xc9,0x1f,0x0a,0x2a,0x5c,0x5d,
0x1f,0xf9,0x08,0x01,0x77,0xb5,0xe8,0x35,0x94,0x34,0x70,0xbd,0xdd,0x86,0xa5,0x00,
0x00,0x04,0x13,0x03,0x13,0x01,0x01,0x00,0x00,0xa4,0x00,0x1a,0x00,0x2c,0xff,0x00,
0x00,0x09,0x00,0x26,0x00,0x01,0x00,0x04,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x04,
0x00,0x04,0x00,0x00,0x00,0x02,0x00,0x04,0x00,0x00,0x00,0x21,0x00,0x08,0x00,0x04,
0x00,0x00,0x00,0x23,0x00,0x03,0x00,0x02,0x00,0x64,0x00,0x2b,0x00,0x03,0x02,0x7f,
0x17,0x00,0x33,0x00,0x26,0x00,0x24,0x00,0x1d,0x00,0x20,0x33,0xc5,0x86,0xee,0x6c
};

const unsigned char cipher_sha256_aes128[] = { // results_3_0
0x32,0x19,0xd6,0x29,0x1b,0x1e,0x80,0x19,0x92,0xc3,0x78,0xce,0x37,0x13,0x62,0x57,
0xa4,0xe6,0x22,0x2d,0xf0,0xc5,0xd4,0x5e,0xe9,0xc9,0x29,0x39,0x8e,0x80,0xcb,0x11,
0xea,0x71,0xd6,0xc1,0xca,0x6a,0xf5,0xd9,0x4e,0xd9,0xac,0x0a,0x38,0xbb,0x53,0x4d,
0xc2,0xc3,0xb6,0x70,0x58,0x7b,0xa8,0x81,0x8a,0x7c,0xa3,0xdf,0xa6,0x12,0x27,0xe3,
0xaf,0x12,0x86,0x9d,0xee,0x9e,0xc4,0x70,0x1b,0xfb,0x06,0x86,0xa1,0xc8,0x31,0xf1,
0x6c,0x25,0x5e,0x07,0x34,0xd1,0xd5,0xb1,0x74,0xdf,0x71,0x1d,0x3c,0xce,0x30,0xbf,
0xd4,0x5b,0x4d,0xe0,0xbf,0x78,0x0d,0x73,0xf8,0x94,0x1e,0x9d,0x19,0x8d,0xdc,0x6b,
0x52,0x08,0xe3,0xc8,0x7e,0xd6,0x78,0xe3,0xd5,0xed,0x0b,0x93,0x5b,0x56,0x33,0x25,
0x5c,0xc4,0x09,0x25,0x50,0x18,0xbb,0x8d,0x36,0x33,0xe2,0x44,0x56,0xce,0x3f,0x74,
0xe7,0xa3,0x31,0xee,0xf0,0x32,0x4e,0x6b,0x00,0x3c,0xb8,0x9a,0xd4,0xb8,0x0b,0xae,
0x39,0x7d,0xfe,0x9b,0xe6,0xd7,0x77,0x94,0xec,0xb3,0xe4,0x41,0x4e,0xbe,0x10,0x6c,
0xb8,0x36,0xf3,0xf7,0x98,0x32,0xa4,0xf7,0x51,0x05,0x0a,0xa6,0x5b,0x0c,0xff,0xa1,
0xcc,0x9f,0x90,0x95,0x22,0xde,0x53,0x5c,0x93,0xac,0xc9,0x6c,0xfa,0xfe,0xa2,0xee,
0x96,0xf4,0x50,0x66,0x74,0xaf,0x3f,0xf3,0x2d,0x49,0xb1,0x25,0xa1,0xeb,0x62,0x89,
0xcf,0xfc,0x71,0xb7,0x98,0xf9,0x56,0xeb,0x83,0x19,0x83,0x77,0x8b,0x26,0x88,0xa5,
0x74,0x5c,0xbf,0xf1,0x20,0xbe,0xd1,0xca,0xfb,0xaa,0x3f,0xd5,0x6c,0xd7,0x9b,0x14,
0xf7,0xbb,0xe6,0xa3,0x2a,0x67,0x35,0x1d,0x35,0xd7,0xf4,0x49,0x7c,0x53,0x30,0x9b,
};

const unsigned char cipher_sha384_aes128[] = { // results_4_0
0x79,0xcf,0x25,0xb3,0x60,0x79,0xa8,0x6c,0x55,0x3b,0xb2,0xcf,0x92,0x58,0x4c,0x14,
0x22,0xec,0xcc,0x2e,0x7f,0x01,0xdf,0x12,0x30,0x49,0x78,0xc7,0x48,0xfa,0x02,0x8c,
0x01,0xda,0xa2,0x2f,0x29,0x50,0x6f,0xe2,0x2f,0xc5,0xde,0x5c,0x3c,0x34,0x36,0x26,
0x59,0x19,0x81,0x30,0x00,0xf5,0x56,0x12,0x77,0x73,0x40,0x1c,0x90,0xb4,0xbb,0x50,
0x18,0x8c,0x9d,0xfa,0x54,0xe6,0xc1,0xc7,0xc1,0xb7,0x81,0x50,0xfc,0xa6,0x6f,0xf6,
0x13,0x94,0x12,0xc4,0x0e,0xdf,0xf2,0x78,0x32,0x61,0xd1,0xf7,0x2b,0x18,0x84,0xb4,
0x0f,0x93,0x60,0xdc,0xe1,0x78,0x6b,0x6f,0x3a,0xe8,0x75,0xe6,0x1e,0x70,0x92,0xdd,
0xf1,0x6e,0x21,0x49,0xfc,0x5b,0xd0,0x90,0x4a,0x46,0xef,0xf1,0x3b,0x60,0x50,0xb9,
0x9a,0x9a,0xab,0xfe,0xa0,0xac,0x4f,0xb7,0xe4,0x87,0x49,0x32,0xdd,0x46,0xae,0xa5,
0xe0,0xb9,0xaa,0x7d,0x25,0x90,0x02,0xfe,0xb0,0xc3,0x71,0xec,0xac,0x59,0x16,0x35,
0x0e,0xbe,0x75,0x2c,0x04,0xa7,0x75,0xf3,0x4b,0x1f,0x53,0x3b,0x00,0x27,0x55,0x95,
0x58,0xac,0x8b,0x2d,0xba,0xc6,0x9c,0x30,0xe8,0xa4,0x8c,0x37,0x02,0xa4,0xb3,0xe6,
0x60,0x01,0x3d,0x9b,0x5e,0x61,0x69,0x5e,0x27,0x1d,0x0c,0x80,0x16,0xf2,0x7b,0x03,
0x2c,0x75,0xb2,0x69,0xcb,0xe0,0x9a,0x14,0xbf,0x7d,0xcd,0x94,0x2f,0x9b,0x36,0xc0,
0x72,0xc6,0xa3,0x31,0xde,0xf5,0x15,0x56,0x23,0x13,0xe2,0x41,0xbc,0xd8,0xa8,0x03,
0xc0,0x05,0x4e,0x91,0xd8,0xaf,0xa6,0x54,0xe3,0x2f,0xb8,0xdf,0x6b,0x96,0x8d,0x5e,
0x99,0x8b,0x97,0x23,0xde,0x8d,0xcf,0x27,0x7b,0x55,0x5c,0xb5,0x9f,0x0f,0x36,0xeb,
};

const unsigned char cipher_sha512_aes128[] = { // results_5_0
0x70,0xe2,0x53,0x71,0xe6,0x13,0xb5,0xa0,0x70,0xe8,0x5e,0x42,0xe2,0xc6,0x18,0xce,
0xb9,0x22,0x5e,0xd3,0xe0,0xb5,0xbb,0x60,0x41,0xae,0x60,0x53,0xca,0x7a,0xae,0xd1,
0xd9,0x90,0xb9,0x79,0x90,0xb8,0x22,0xae,0xaa,0x26,0x90,0xe4,0x97,0xf0,0xd0,0xa4,
0xe9,0x83,0xf4,0x80,0xf1,0x69,0xaf,0x34,0x25,0x96,0xb0,0x74,0x8c,0x32,0xd0,0x02,
0x62,0x14,0x75,0x9e,0x54,0xf9,0x4d,0xbd,0xbf,0x0c,0xaf,0xca,0x58,0xe8,0xa1,0xbf,
0x48,0x34,0x5c,0xdf,0x28,0x06,0x58,0x4b,0xd8,0x63,0x95,0xb2,0x7f,0x3b,0x81,0xfe,
0x0b,0x03,0x2f,0x13,0xeb,0x6d,0x84,0xbd,0xcc,0x85,0x98,0x2d,0x7e,0x3c,0x98,0xc5,
0x16,0x94,0xbe,0x02,0xe7,0x22,0x11,0x1a,0xa4,0x5e,0x50,0xaa,0x63,0x85,0x84,0x76,
0x2b,0x2a,0x80,0x8d,0x7c,0xa9,0xf8,0x17,0x17,0x5a,0x75,0x83,0x25,0xa5,0x96,0xd3,
0x69,0xd3,0x46,0x19,0x9c,0x78,0x00,0xd3,0x52,0xe5,0x1a,0xb9,0xe2,0x55,0x12,0xa7,
0xb0,0xe2,0xf9,0x53,0x08,0x49,0x77,0x2d,0x6a,0x05,0x83,0x83,0x3c,0x56,0xf1,0xc2,
0x93,0x70,0xc2,0x4c,0x1a,0x05,0xa0,0x3d,0xa3,0xa7,0x26,0xf1,0x7d,0xfc,0x14,0x68,
0x11,0x9c,0x08,0xbd,0x7f,0x4d,0xd7,0x68,0x52,0xda,0xfb,0x33,0x01,0x27,0x2a,0x15,
0x76,0x27,0x6d,0x60,0xcd,0x51,0xf3,0xf7,0x8e,0xfa,0x27,0x40,0x8a,0x0f,0xf6,0x33,
0x6a,0x44,0x5f,0x4c,0xe6,0x90,0xdc,0x78,0x7b,0x04,0xdc,0xda,0xf5,0xf9,0x59,0xb3,
0x07,0xa0,0x90,0x44,0x70,0x86,0x91,0xb5,0x1c,0xef,0x3b,0x49,0x27,0x8e,0x85,0xb4,
0x0b,0x3e,0x9d,0xc5,0x95,0x2e,0x67,0xd9,0xaf,0x3f,0x66,0x06,0xc1,0x75,0xfe,0xcd,
};

const unsigned char cipher_sha256_aes256[] = { // results_3_1
0x83,0xb4,0xe5,0xbd,0x4d,0x04,0xaf,0x07,0x23,0x84,0xb9,0xd7,0x5a,0xa8,0x54,0x06,
0xc4,0xb1,0xb2,0x76,0xc7,0x64,0x2b,0x06,0x51,0x18,0x31,0x1b,0xd8,0x26,0xbb,0xc0,
0xa6,0x63,0xaf,0x72,0x76,0x89,0x5f,0x9d,0x71,0x6f,0x89,0xf6,0x8d,0x38,0xd0,0xce,
0xdc,0x79,0x7c,0x05,0x11,0x62,0x2a,0xfb,0xb8,0x42,0x79,0x9f,0xc7,0x20,0x6f,0x7a,
0x30,0xdb,0x01,0xd2,0xe8,0x50,0x37,0x23,0xf5,0x9d,0x83,0x0c,0x24,0xba,0xeb,0x1a,
0x70,0xc3,0x01,0xfa,0x43,0xf7,0x18,0xdb,0xb6,0xf7,0xa4,0x76,0x2c,0x0d,0x11,0x9d,
0x06,0x5b,0xc5,0x22,0xfa,0x62,0xb6,0x08,0xc9,0x41,0x0f,0x12,0x26,0xac,0x21,0xde,
0x1c,0x7b,0x6a,0x1c,0x45,0xf2,0xe9,0xf4,0x51,0x41,0x16,0x10,0xf5,0x2f,0x71,0x15,
0x24,0x8c,0x14,0xba,0xad,0xf9,0x5d,0x6b,0x0d,0x05,0x81,0xa6,0xd0,0x77,0x0f,0x69,
0x79,0x50,0x32,0x21,0x27,0xf2,0xc5,0x51,0x44,0xd3,0xea,0x70,0xb4,0xd4,0xcc,0x48,
0xe2,0xe6,0x89,0x9d,0x9e,0x2f,0x72,0x1e,0x7a,0xc6,0x6b,0xbe,0xa3,0x08,0x4a,0x04,
0x1d,0x6e,0x5e,0x0e,0xb8,0x67,0x6d,0x1f,0xad,0x62,0xcb,0x78,0x67,0x25,0xdc,0x3a,
0x90,0xb2,0xc1,0xc6,0xf2,0x0a,0xda,0xa7,0x5a,0xc0,0x00,0xd0,0xb0,0xe0,0xf8,0xc9,
0x5c,0xfd,0xe8,0x00,0x2b,0x07,0x32,0x34,0x04,0xaf,0x29,0x26,0x55,0x93,0x3f,0xb0,
0x8a,0x0e,0x81,0xb5,0xc2,0x36,0x8e,0xf2,0x3c,0xa7,0x95,0xc7,0x29,0xf4,0x4c,0xfe,
0x5c,0x2b,0x74,0xf1,0x85,0xf2,0xff,0xb9,0x7a,0x5c,0x17,0xa2,0xbc,0x00,0x47,0x33,
0x5a,0x1a,0x31,0x89,0xc1,0xc8,0x08,0x2e,0xb2,0x37,0x57,0x2b,0x2b,0xa1,0x46,0xfb,
};

const unsigned char cipher_sha384_aes256[] = { // results_4_1
0x78,0x76,0x49,0x48,0x08,0x5b,0x9b,0x95,0xda,0xc6,0xb1,0xfe,0xbf,0x50,0x4a,0xa2,
0xce,0xd9,0xa1,0xa6,0x1d,0x16,0xd8,0xa1,0x74,0xc7,0x4c,0x07,0x8b,0xd4,0xcf,0xa4,
0x90,0x8a,0x87,0x75,0x26,0x93,0xb6,0xa1,0x02,0x7b,0x11,0x3f,0x71,0xe8,0x38,0xdb,
0xf6,0xf5,0x0a,0xbf,0xea,0x99,0xb8,0xa3,0x29,0xc1,0x7b,0x95,0x85,0x60,0x5a,0x21,
0xf3,0xcf,0x75,0xf4,0xc6,0x9c,0x30,0x19,0xed,0xd1,0x50,0xde,0x63,0x41,0x56,0x63,
0x2f,0x1f,0x38,0x71,0xd0,0xa9,0xc1,0x2c,0x09,0xb8,0xa2,0xd6,0xc5,0x54,0x1a,0x3d,
0xf3,0xb8,0x69,0x61,0x76,0xf6,0xdf,0xec,0x01,0x93,0x84,0x85,0x39,0x09,0x9e,0x16,
0xcd,0x3f,0x25,0x27,0x58,0x21,0x9f,0x48,0xed,0x37,0x45,0x33,0xe3,0x83,0x65,0x26,
0x8e,0xb5,0x5c,0x52,0xcd,0x94,0xec,0xf4,0x09,0xbc,0x88,0xa2,0x62,0x04,0x4a,0x98,
0x3c,0xbe,0xca,0x7b,0xe1,0x42,0xc2,0x34,0x3f,0x0a,0x2a,0xe4,0xf4,0xac,0x10,0xa8,
0xbb,0x15,0x83,0x58,0xb1,0x8f,0xda,0xb3,0xf3,0x37,0x94,0x90,0x8b,0xde,0x97,0xb9,
0x8c,0xb9,0x2f,0xbd,0x38,0x50,0xaf,0xff,0x99,0xcb,0x9f,0xa4,0xfd,0xa3,0xea,0xd2,
0x58,0xda,0x93,0xd9,0x4c,0xad,0xc2,0xd5,0xc1,0x58,0x53,0xfd,0xb7,0xad,0xd2,0x9a,
0xb3,0xd7,0x28,0x32,0xda,0xbc,0x73,0xc5,0x5c,0x02,0x6a,0x54,0xc0,0xdb,0x11,0x51,
0x5c,0x58,0xc9,0x16,0xc7,0x31,0x62,0xb5,0x5c,0xb2,0xa6,0xec,0x4c,0x2e,0xa4,0x99,
0x52,0x8c,0xa7,0xf7,0x06,0x95,0x48,0x9a,0x50,0xe6,0xde,0x00,0xbb,0xba,0x19,0x0d,
0x65,0x16,0x88,0xc5,0x74,0x4c,0x80,0xad,0x31,0xff,0x4e,0x73,0x4c,0x1a,0xcd,0x33,
};

const unsigned char cipher_sha512_aes256[] = { // results_5_1
0xb1,0xb4,0x0b,0x35,0x10,0x26,0xb2,0x4f,0x37,0xf9,0xbd,0xef,0x34,0x3a,0x4f,0xb6,
0x19,0xa9,0x2a,0xd3,0x3a,0x6a,0x5d,0x21,0x3e,0x88,0xa5,0xce,0x1b,0x65,0xd0,0x91,
0xf2,0x3d,0x01,0x92,0x10,0xc9,0xa4,0xcc,0x2f,0x99,0x07,0x2c,0x8a,0xfe,0xf5,0x54,
0x8c,0x97,0x7d,0xf0,0x4e,0x67,0x6c,0x2a,0x44,0x3f,0x29,0xdf,0xcf,0x9f,0xae,0xfe,
0x1f,0x71,0xb0,0xad,0x5d,0x0d,0x05,0x34,0xbd,0x9c,0x84,0x20,0x18,0xc8,0xa9,0x74,
0x36,0xda,0x3c,0x31,0xb0,0x63,0xcc,0x6c,0xcc,0xcd,0xb5,0xf8,0x7d,0xc2,0xcd,0xb1,
0x7d,0x24,0xb8,0x9f,0x27,0x7f,0x8e,0x7c,0xa0,0x47,0x22,0xdf,0x62,0xd3,0xdd,0x91,
0xea,0x36,0x1c,0xfc,0x5b,0x4a,0xd6,0x1c,0x6e,0x74,0x9c,0xba,0xdf,0x1d,0x37,0x06,
0xba,0x4c,0x7f,0xe6,0x38,0x8b,0xfb,0xb5,0x84,0x38,0xd2,0x91,0x07,0x7e,0xe7,0xdb,
0x99,0xfc,0x79,0xa3,0xc8,0xc0,0x81,0x53,0xc9,0xf6,0x6b,0x22,0x48,0x26,0x2d,0xfd,
0x1f,0x01,0x8f,0x3e,0xd3,0x19,0xf7,0x70,0x8f,0x1f,0x51,0xb7,0xfd,0xbe,0x43,0x5e,
0x05,0x7a,0x8e,0xfa,0xa4,0x15,0x86,0xf2,0x77,0xb4,0xcf,0x85,0xc3,0x38,0xee,0x1f,
0xa0,0x5e,0x52,0x3e,0x07,0x00,0x15,0xf4,0xc6,0x48,0xc2,0x98,0x0b,0x27,0xb0,0x57,
0x33,0x82,0x28,0x50,0xb4,0x85,0x70,0x2c,0x03,0x47,0xeb,0x72,0x22,0x4e,0xcf,0xe0,
0xec,0x32,0xce,0x9a,0x68,0x3e,0x04,0xb8,0x3b,0x04,0x6f,0x69,0x0f,0x25,0xa4,0x83,
0x13,0x06,0x45,0x7e,0x9d,0xc1,0xfb,0x87,0x83,0x52,0x8e,0x39,0x00,0x91,0x4f,0xf0,
0x10,0x7f,0xd8,0x8c,0x88,0x92,0x3b,0xa8,0x89,0xaa,0x31,0xbe,0x27,0xef,0x91,0xf5,
};

const unsigned char cipher_sha256_chachapoly[] = { // results_3_2
0x93,0xa1,0xb8,0x55,0xb4,0xc4,0xc5,0x40,0xe1,0x22,0x64,0xde,0x5a,0x57,0xba,0x19,
0x08,0x0c,0x0b,0x30,0xde,0x17,0x17,0x8c,0xf2,0x3d,0x66,0x80,0x61,0x59,0xea,0x71,
0xaf,0x1e,0xd3,0x87,0x48,0xb7,0x43,0x0e,0x75,0x31,0xf7,0x1f,0xb2,0xf4,0x27,0x1f,
0x48,0x74,0xb5,0xa9,0xa8,0xcb,0x4b,0xeb,0x0a,0x93,0xcb,0x0e,0x72,0x11,0x9b,0x3b,
0x89,0x4b,0xb9,0x80,0x72,0x0e,0xd8,0x83,0x73,0x28,0x9d,0x1e,0xe8,0x62,0x0a,0x83,
0x61,0x72,0x83,0x44,0x58,0xb7,0xb7,0x30,0x4f,0xf3,0x85,0xb7,0x59,0x6b,0x49,0xef,
0xc2,0xd4,0x48,0x77,0x3b,0x68,0x7c,0x17,0xb9,0x6a,0x91,0x1f,0xc6,0x27,0x5a,0x21,
0x5a,0x95,0x46,0x5e,0xba,0x52,0x27,0xb3,0xb2,0xd2,0x5b,0x74,0xe4,0xd9,0x3e,0xb4,
0xef,0x65,0xa9,0xde,0x25,0xe8,0x55,0x4e,0xa8,0xa6,0x46,0xdb,0xf4,0xc1,0xde,0x3e,
0x69,0x78,0x85,0x3f,0xa9,0xbb,0xb8,0x76,0x33,0x11,0xbf,0xcb,0x8b,0xea,0xec,0x4d,
0xd6,0xdd,0x79,0x70,0x8e,0x29,0x1b,0xa9,0x87,0x2a,0x2f,0x16,0xae,0x74,0xa4,0x4b,
0x07,0x47,0x69,0xcc,0x1a,0x9d,0xad,0xbd,0xb7,0x71,0x77,0x1b,0x0b,0x49,0x3c,0x99,
0x46,0x1a,0x0a,0x54,0x04,0xd6,0xb9,0x26,0x54,0x14,0x09,0x70,0xfd,0xbe,0x52,0xef,
0x37,0xaf,0x3c,0x9c,0x2e,0x23,0x85,0x17,0x32,0x52,0x25,0x25,0xff,0xa5,0x76,0x66,
0x84,0xed,0x1d,0x6d,0xda,0x29,0x8b,0x30,0x56,0x8b,0x0b,0x42,0x4e,0x20,0xe8,0x87,
0x3c,0xcf,0x7c,0xce,0xd6,0x13,0x04,0x41,0xcf,0x45,0x73,0x2f,0x98,0x3c,0x4e,0x81,
0xc9,0xf9,0xfd,0xf9,0x01,0x78,0x17,0x67,0x7d,0x5c,0xe6,0xd8,0xb3,0x4d,0xee,0xf9,
};

const unsigned char cipher_sha384_chachapoly[] = { // results_4_2
0x7c,0x9d,0x7d,0xd7,0x4a,0x42,0x37,0x68,0xb9,0x9c,0x4e,0x4f,0x87,0x23,0x8a,0x71,
0xd7,0x3c,0xfe,0x13,0xaa,0x18,0x42,0x5d,0xbf,0x12,0x83,0x53,0x99,0x73,0xc6,0xeb,
0xeb,0x3e,0x49,0xec,0x5d,0xcf,0x13,0x05,0x78,0x43,0xd3,0xfa,0xcb,0x04,0xe3,0x6e,
0xec,0x86,0x27,0x6d,0x9a,0x5d,0xe1,0xb9,0xd2,0x40,0x5e,0x20,0x64,0x78,0x19,0x13,
0x72,0x46,0xe5,0x5e,0xe5,0x47,0x7e,0x70,0x17,0x4d,0x91,0x5d,0xb1,0xd3,0x86,0x40,
0x95,0xce,0x8f,0x55,0x8a,0xe1,0xa5,0x00,0x6b,0x67,0x94,0x4e,0x1e,0xc9,0x2f,0x45,
0x25,0x1e,0x1b,0x9c,0xec,0x44,0x55,0x16,0x4f,0x93,0x12,0x73,0x28,0x24,0x03,0x32,
0xee,0x9d,0xff,0xa4,0x9f,0x1e,0x80,0x1e,0x4b,0x05,0x58,0x06,0x41,0x3a,0xd2,0x66,
0x45,0x71,0x66,0xe7,0x95,0x7f,0xec,0xdd,0x87,0x05,0x15,0x08,0x46,0x8c,0x5c,0xc5,
0xbd,0x27,0xac,0x3e,0x79,0xb0,0x4a,0xeb,0xc8,0x28,0xe2,0xba,0x5a,0x3d,0x79,0xc8,
0xa1,0x05,0x37,0x9e,0xb3,0xb0,0x11,0x0d,0xd4,0x43,0xe7,0x4a,0xaf,0xb5,0xe8,0xfe,
0xfb,0x5d,0xf5,0x65,0x9a,0x29,0x94,0x11,0xd2,0x09,0xd8,0x7a,0x95,0xb2,0x21,0x8c,
0x3b,0x68,0x80,0x3d,0xe3,0x25,0xa5,0xe2,0xfc,0xbb,0xd7,0xe3,0x58,0xa5,0x06,0x9c,
0xf8,0xbe,0x24,0xbf,0x5a,0x23,0x4d,0xd1,0x38,0x04,0x0b,0xae,0xb1,0x22,0x2d,0x3b,
0xa7,0x2b,0xf8,0x42,0x67,0xdd,0xfd,0x0c,0x3f,0xcd,0xfc,0xc7,0xd7,0xc1,0xb6,0x83,
0xc6,0x1e,0x22,0x9e,0xf7,0x27,0x7f,0x05,0x7d,0x0d,0x18,0x51,0x34,0x48,0x89,0x76,
0x34,0xa9,0x9a,0xae,0xfc,0x51,0xc8,0xdc,0xbb,0xc4,0x64,0xa1,0x97,0x95,0x1e,0x52
};

const unsigned char cipher_sha512_chachapoly[] = { // results_5_2
0x1d,0xae,0x2c,0xf0,0x24,0x3f,0x6d,0xfb,0x22,0x8d,0xe1,0xfe,0xab,0xbb,0xe8,0x27,
0xa6,0x98,0x3f,0x86,0x78,0x3e,0x28,0x0e,0x2c,0xa7,0xb8,0x46,0x98,0x25,0x09,0x35,
0x34,0x4f,0x5f,0x50,0xbd,0xad,0x0d,0x37,0x1a,0x43,0x53,0x59,0x49,0x5d,0xac,0x1a,
0x0d,0xf3,0x5b,0xf1,0x2d,0x29,0x56,0x9e,0xb3,0xd4,0x22,0x31,0x1d,0x0f,0xa9,0xc3,
0xf9,0x30,0x38,0x1d,0xf3,0x28,0x1b,0x7f,0xef,0x67,0x6f,0x69,0xf1,0x34,0x81,0x5b,
0x11,0xf2,0x62,0xf4,0x37,0x1d,0x4b,0x54,0x4c,0x1e,0x8f,0xe4,0x25,0xde,0x28,0xda,
0x56,0xdd,0xc6,0x71,0xe1,0xb5,0xf5,0xe5,0x67,0x8a,0x05,0x8e,0x30,0x74,0x36,0x40,
0x05,0xd7,0x06,0x7f,0x44,0x29,0xff,0x08,0x67,0x4d,0xfb,0xf3,0xc9,0x77,0xe5,0xc7,
0xc0,0x5e,0x2e,0xf8,0x80,0xcb,0x2d,0xee,0x15,0x16,0x33,0xcc,0xa3,0xc5,0x88,0x8e,
0x91,0xee,0x6f,0xf1,0x05,0xd9,0xbe,0x5d,0x97,0xae,0x7c,0x7e,0x85,0x65,0xea,0x70,
0xa2,0xb1,0x93,0x10,0x03,0xcf,0xa9,0x56,0x08,0x9c,0xd1,0x60,0xf9,0xad,0x1a,0x7e,
0x44,0xdb,0xac,0x01,0x04,0x6c,0x09,0x46,0xec,0xc5,0x51,0x21,0x22,0x6e,0xc8,0xbb,
0x39,0xbe,0x4d,0x8b,0x83,0x09,0x8c,0x41,0xac,0x21,0x7f,0x54,0x01,0x25,0x17,0x27,
0x4b,0x08,0xfd,0xc4,0xac,0x79,0x91,0x5d,0x7b,0xd8,0x7d,0xe8,0x47,0xc2,0x89,0x6c,
0x21,0xb3,0xfc,0x18,0xfc,0x27,0x4f,0x18,0xa5,0xe6,0xbb,0x73,0xa3,0xde,0x45,0x56,
0xf6,0x1b,0x11,0x10,0x10,0x5f,0xb8,0x1c,0x6e,0x39,0x25,0xd6,0x1e,0x52,0x07,0xdd,
0xf2,0x4e,0xc4,0x23,0x59,0x00,0x1d,0xc1,0xaa,0x55,0x9a,0xff,0x86,0xaa,0xa1,0xf9,
};

struct testcombination {
    mitls_hash hash;
    mitls_aead ae;
    const unsigned char *expected_cipher;
} testcombinations[] = {
    // TLS_hash_MD5 and TLS_hash_SHA1 aren't supported by libquiccrypto

    { TLS_hash_SHA256, TLS_aead_AES_128_GCM, cipher_sha256_aes128 },
    { TLS_hash_SHA384, TLS_aead_AES_128_GCM, cipher_sha384_aes128 },
//    { TLS_hash_SHA512, TLS_aead_AES_128_GCM, cipher_sha512_aes128 },

    { TLS_hash_SHA256, TLS_aead_AES_256_GCM, cipher_sha256_aes256 },
    { TLS_hash_SHA384, TLS_aead_AES_256_GCM, cipher_sha384_aes256 },
//    { TLS_hash_SHA512, TLS_aead_AES_256_GCM, cipher_sha512_aes256 },

    { TLS_hash_SHA256, TLS_aead_CHACHA20_POLY1305, cipher_sha256_chachapoly },
    { TLS_hash_SHA384, TLS_aead_CHACHA20_POLY1305, cipher_sha384_chachapoly },
//    { TLS_hash_SHA512, TLS_aead_CHACHA20_POLY1305, cipher_sha512_chachapoly },

    { 0, 0, NULL }
};

// map mitls_hash to a string name
const char *hash_to_name[] = {
    "TLS_hash_MD5",
    "TLS_hash_SHA1",
    "TLS_hash_SHA224",
    "TLS_hash_SHA256",
    "TLS_hash_SHA384",
    "TLS_hash_SHA512"
};

// map mitls_aead to a string name
const char *aead_to_name[] = {
    "TLS_aead_AES_128_GCM",
    "TLS_aead_AES_256_GCM",
    "TLS_aead_CHACHA20_POLY1305"
};

void dumptofile(FILE *fp, const char buffer[], size_t len)
{
  size_t i;
  for(i=0; i<len; i++) {
    fprintf(fp, "0x%2.2x,", (unsigned char)buffer[i]);
    if (i % 16 == 15 || i == len-1) fprintf(fp, "\n");
  }
}

void test_crypto(const quic_secret *secret, const unsigned char *expected_cipher)
{
    int result;
    quic_key *key;
    unsigned char cipher[plain_len+16];

    printf("==== test_crypto(%s,%s) ====\n",
        hash_to_name[secret->hash], aead_to_name[secret->ae]);

    result = quic_crypto_derive_key(&key, secret);
    if (result == 0) {
        printf("FAIL: quic_crypto_derive_key failed\n");
        exit(1);
    }

    memset(cipher, 0, sizeof(cipher));
    result = quic_crypto_encrypt(key, cipher, sn, ad, ad_len, plain, plain_len);
    if (result == 0) {
        printf("FAIL: quic_crypto_encrypt failed\n");
        exit(1);
    }

#if 0
    // Capture expected results to files ready to paste into C source code
    char fname[64];
    sprintf(fname, "results_%d_%d", secret->hash, secret->ae);
    FILE *fp = fopen(fname, "w");
    dumptofile(fp, cipher, sizeof(cipher));
    fclose(fp);
#else
    // Verify that the computed text matches expectations
    check_result("quic_crypto_encrypt", cipher, expected_cipher, sizeof(cipher));
#endif

    unsigned char decrypted[plain_len];
    result = quic_crypto_decrypt(key, decrypted, sn, ad, ad_len, cipher, sizeof(cipher));
    if (result == 0) {
        printf("FAIL: quic_crypto_decrypt failed\n");
        exit(1);
    }
    check_result("quic_crypto_decrypt", decrypted, plain, sizeof(decrypted));

    result = quic_crypto_free_key(key);
    if (result == 0) {
        printf("FAIL: quic_crypto_free_key failed\n");
        exit(1);
    }

    printf("PASS\n");
}

const uint8_t expected_client_hs[] =  { // client_hs (draft 13)
0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xb7,0xf3,0x82,0xbf,0x3e,0xd3,0x28,0xa7,
0xb7,0xa3,0x24,0xdb,0x6b,0x01,0xb9,0xd1,0x48,0x90,0x62,0x63,0x5f,0x3f,0x51,0x1d,
0xb9,0xc9,0x69,0xec,0xcd,0x8a,0xe1,0x9a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const uint8_t expected_server_hs[] = { // server_hs (draft 13)
0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x57,0x9d,0x38,0x7e,0x60,0x84,0x52,0x99,
0xab,0x28,0x9a,0x26,0x2f,0x44,0xb5,0x0c,0x9c,0x6f,0xb4,0x9a,0x16,0x89,0x62,0x73,
0xe7,0x97,0x86,0x3c,0x11,0x44,0x64,0xf1,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void test_pn_encrypt()
{
  printf("==== test_pn_encrypt() ====\n");

  quic_secret client_hs;
  memset(&client_hs, 0, sizeof(client_hs));

  quic_secret server_hs;
  memset(&server_hs, 0, sizeof(server_hs));

  quic_key *key;
  int result, i;

  static const uint8_t cid[] = { 0x77, 0x0d, 0xc2, 0x6c, 0x17, 0x50, 0x9b, 0x35 };
  static const uint8_t salt[] = { 0x9c, 0x10, 0x8f, 0x98, 0x52, 0x0a, 0x5c, 0x5c, 0x32, 0x96, 0x8e, 0x95, 0x0e, 0x8a, 0x2c, 0x5f, 0xe0, 0x6d, 0x6c, 0x38 };
  static const uint8_t sample[] = { 0x05, 0x80, 0x24, 0xa9, 0x72, 0x75, 0xf0, 0x1d, 0x2a, 0x1e, 0xc9, 0x1f, 0xd1, 0xc2, 0x65, 0xbb };
  static const uint8_t encrypted_pn[] = { 0x3b, 0xb4, 0xb1, 0x74 };
  static const uint8_t expected_pn[] = { 0xf9, 0xd8, 0x57, 0xaa };
  uint8_t pnmask[4] = {0};

  result = quic_derive_initial_secrets(&client_hs, &server_hs, cid, sizeof(cid), salt, sizeof(salt));
  if (result == 0) {
      printf("FAIL: quic_derive_initial_secrets failed\n");
      exit(1);
  }

  result = quic_crypto_derive_key(&key, &client_hs);
  if (result == 0) {
      printf("FAIL: quic_crypto_derive_key failed\n");
      exit(1);
  }

  if(quic_crypto_packet_number_otp(key, sample, pnmask))
  {
    printf("PN encryption mask: "); dump(pnmask, 4);
    for(i = 0; i < 4; i++) pnmask[i] ^= encrypted_pn[i];
    printf("Decrypted PN: "); dump(pnmask, 4);
    check_result("decrypted PN", pnmask, expected_pn, sizeof(pnmask));
  } else {
    printf("PN encryption failed.\n");
    exit(1);
  }
}

void test_initial_secrets()
{
    int result;

    printf("==== test_initial_secrets() ====\n");

    quic_secret client_hs;
    memset(&client_hs, 0, sizeof(client_hs));

    quic_secret server_hs;
    memset(&server_hs, 0, sizeof(server_hs));

    const unsigned char con_id[12] = {0xff,0xaa,0x55,0x00, 0x80,0x01,0x7f,0xee, 0x81,0x42,0x24,0x18 };
    const unsigned char salt[] = {0xaf,0xc8,0x24,0xec,0x5f,0xc7,0x7e,0xca,0x1e,0x9d,0x36,0xf3,0x7f,0xb2,0xd4,0x65,0x18,0xc3,0x66,0x39};
    result = quic_derive_initial_secrets(&client_hs, &server_hs, con_id, sizeof(con_id), salt, sizeof(salt));
    if (result == 0) {
        printf("FAIL: quic_derive_initial_secrets failed\n");
        exit(1);
    }

#if 0
    // Capture expected results to files ready to paste into C source code
    FILE *fp = fopen("client_hs", "w");
    dumptofile(fp, (const uint8_t*)&client_hs, sizeof(client_hs));
    fclose(fp);
    fp = fopen("server_hs", "w");
    dumptofile(fp, (const uint8_t*)&server_hs, sizeof(server_hs));
    fclose(fp);
#else
    // Verify that the computed text matches expectations
    check_result("quic_derive_initial_secrets client", (const unsigned char*)&client_hs, (const unsigned char*)&expected_client_hs, sizeof(client_hs));
    check_result("quic_derive_initial_secrets server", (const unsigned char*)&server_hs, (const unsigned char*)&expected_server_hs, sizeof(server_hs));
#endif

    printf("==== PASS: test_initial_secrets ==== \n");
}

void exhaustive(void)
{
    quic_secret secret;

    for (unsigned char i=0; i<sizeof(secret.secret); ++i) {
        secret.secret[i] = i;
    }

    for (size_t i=0; testcombinations[i].expected_cipher; ++i) {
        secret.hash = testcombinations[i].hash;
        secret.ae = testcombinations[i].ae;

        test_crypto(&secret, testcombinations[i].expected_cipher);
    }

    test_pn_encrypt();
    test_initial_secrets();
}

int CDECL main(int argc, char **argv)
{
    // Reference arguments to avoid compiler errors
    (void)argc;
    (void)argv;

    EverCrypt_AutoConfig2_init();

    coverage();
    exhaustive();

    printf("==== ALL TESTS PASS ====\n");
}
