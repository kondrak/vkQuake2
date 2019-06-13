/*
 * Public Domain C source implementation of RFC 1320
 *       -  The MD4 Message-Digest Algorithm  -
 *
 *        http://www.faqs.org/rfcs/rfc1320.html
 *                    by Steven Fuller
 */

#include <inttypes.h>

#define ROTATELEFT32(x, s) (((x) << (s)) | ((x) >> (32 - (s))))

#define F(X, Y, Z) (((X)&(Y)) | ((~X) & (Z)))
#define G(X, Y, Z) (((X)&(Y)) | ((X)&(Z)) | ((Y)&(Z)))
#define H(X, Y, Z) ((X) ^ (Y) ^ (Z))

#define S(a, b, c, d, k, s)	\
	{ \
		a += (F((b), (c), (d)) + X[(k)]); \
		a = ROTATELEFT32(a, s);	\
	}
#define T(a, b, c, d, k, s)	\
	{ \
		a += (G((b), (c), (d)) + X[(k)] + 0x5A827999); \
		a = ROTATELEFT32(a, s);	\
	}
#define U(a, b, c, d, k, s)	\
	{ \
		a += (H((b), (c), (d)) + X[(k)] + 0x6ED9EBA1); \
		a = ROTATELEFT32(a, s);	\
	}

static uint32_t X[16];
static uint32_t A, AA;
static uint32_t B, BB;
static uint32_t C, CC;
static uint32_t D, DD;

static void
DoMD4()
{
	AA = A;
	BB = B;
	CC = C;
	DD = D;

	S(A, B, C, D, 0, 3);
	S(D, A, B, C, 1, 7);
	S(C, D, A, B, 2, 11);
	S(B, C, D, A, 3, 19);
	S(A, B, C, D, 4, 3);
	S(D, A, B, C, 5, 7);
	S(C, D, A, B, 6, 11);
	S(B, C, D, A, 7, 19);
	S(A, B, C, D, 8, 3);
	S(D, A, B, C, 9, 7);
	S(C, D, A, B, 10, 11);
	S(B, C, D, A, 11, 19);
	S(A, B, C, D, 12, 3);
	S(D, A, B, C, 13, 7);
	S(C, D, A, B, 14, 11);
	S(B, C, D, A, 15, 19);

	T(A, B, C, D, 0, 3);
	T(D, A, B, C, 4, 5);
	T(C, D, A, B, 8, 9);
	T(B, C, D, A, 12, 13);
	T(A, B, C, D, 1, 3);
	T(D, A, B, C, 5, 5);
	T(C, D, A, B, 9, 9);
	T(B, C, D, A, 13, 13);
	T(A, B, C, D, 2, 3);
	T(D, A, B, C, 6, 5);
	T(C, D, A, B, 10, 9);
	T(B, C, D, A, 14, 13);
	T(A, B, C, D, 3, 3);
	T(D, A, B, C, 7, 5);
	T(C, D, A, B, 11, 9);
	T(B, C, D, A, 15, 13);

	U(A, B, C, D, 0, 3);
	U(D, A, B, C, 8, 9);
	U(C, D, A, B, 4, 11);
	U(B, C, D, A, 12, 15);
	U(A, B, C, D, 2, 3);
	U(D, A, B, C, 10, 9);
	U(C, D, A, B, 6, 11);
	U(B, C, D, A, 14, 15);
	U(A, B, C, D, 1, 3);
	U(D, A, B, C, 9, 9);
	U(C, D, A, B, 5, 11);
	U(B, C, D, A, 13, 15);
	U(A, B, C, D, 3, 3);
	U(D, A, B, C, 11, 9);
	U(C, D, A, B, 7, 11);
	U(B, C, D, A, 15, 15);

	A += AA;
	B += BB;
	C += CC;
	D += DD;
}

static void
PerformMD4(const unsigned char *buf, int length, unsigned char *digest)
{
	int len = length / 64; /* number of full blocks */
	int rem = length % 64; /* number of left over bytes */

	int i, j;
	const unsigned char *ptr = buf;

	/* initialize the MD buffer */
	A = 0x67452301;
	B = 0xEFCDAB89;
	C = 0x98BADCFE;
	D = 0x10325476;

	for (i = 0; i < len; i++)
	{
		for (j = 0; j < 16; j++)
		{
			X[j] = ((ptr[0] << 0) | (ptr[1] << 8) |
					(ptr[2] << 16) | (ptr[3] << 24));

			ptr += 4;
		}

		DoMD4();
	}

	i = rem / 4;

	for (j = 0; j < i; j++)
	{
		X[j] = ((ptr[0] << 0) | (ptr[1] << 8) |
				(ptr[2] << 16) | (ptr[3] << 24));

		ptr += 4;
	}

	switch (rem % 4)
	{
		case 0:
			X[j] = 0x80U;
			break;
		case 1:
			X[j] = ((ptr[0] << 0) | ((0x80U) << 8));
			break;
		case 2:
			X[j] = ((ptr[0] << 0) | (ptr[1] << 8) | ((0x80U) << 16));
			break;
		case 3:
			X[j] =
				((ptr[0] <<
				  0) | (ptr[1] << 8) | (ptr[2] << 16) | ((0x80U) << 24));
			break;
	}

	j++;

	if (j > 14)
	{
		for ( ; j < 16; j++)
		{
			X[j] = 0;
		}

		DoMD4();

		j = 0;
	}

	for ( ; j < 14; j++)
	{
		X[j] = 0;
	}

	X[14] = (length & 0x1FFFFFFF) << 3;
	X[15] = (length & ~0x1FFFFFFF) >> 29;

	DoMD4();

	digest[0] = (A & 0x000000FF) >> 0;
	digest[1] = (A & 0x0000FF00) >> 8;
	digest[2] = (A & 0x00FF0000) >> 16;
	digest[3] = (A & 0xFF000000) >> 24;
	digest[4] = (B & 0x000000FF) >> 0;
	digest[5] = (B & 0x0000FF00) >> 8;
	digest[6] = (B & 0x00FF0000) >> 16;
	digest[7] = (B & 0xFF000000) >> 24;
	digest[8] = (C & 0x000000FF) >> 0;
	digest[9] = (C & 0x0000FF00) >> 8;
	digest[10] = (C & 0x00FF0000) >> 16;
	digest[11] = (C & 0xFF000000) >> 24;
	digest[12] = (D & 0x000000FF) >> 0;
	digest[13] = (D & 0x0000FF00) >> 8;
	digest[14] = (D & 0x00FF0000) >> 16;
	digest[15] = (D & 0xFF000000) >> 24;

	A = AA = 0;
	B = BB = 0;
	C = CC = 0;
	D = DD = 0;

	for (j = 0; j < 16; j++)
	{
		X[j] = 0;
	}
}

unsigned
Com_BlockChecksum(void *buffer, int length)
{
	uint32_t digest[4];
	unsigned val;

	PerformMD4((unsigned char *)buffer, length, (unsigned char *)digest);

	val = digest[0] ^ digest[1] ^ digest[2] ^ digest[3];

	return val;
}
