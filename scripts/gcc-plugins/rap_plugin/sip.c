// SipHash-2-4 adapted by the PaX Team from the public domain version written by
//   Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
//   Daniel J. Bernstein <djb@cr.yp.to>

#include <stdint.h>

#define ROTL(x, b) (u64)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)						\
	(p)[0] = (u8)((v)      ); (p)[1] = (u8)((v) >>  8);	\
	(p)[2] = (u8)((v) >> 16); (p)[3] = (u8)((v) >> 24);

#define U64TO8_LE(p, v)				\
	U32TO8_LE((p),     (u32)((v)      ));	\
	U32TO8_LE((p) + 4, (u32)((v) >> 32));

#define U8TO64_LE(p)	(	\
	((u64)((p)[0])      ) |	\
	((u64)((p)[1]) <<  8) |	\
	((u64)((p)[2]) << 16) |	\
	((u64)((p)[3]) << 24) |	\
	((u64)((p)[4]) << 32) |	\
	((u64)((p)[5]) << 40) |	\
	((u64)((p)[6]) << 48) |	\
	((u64)((p)[7]) << 56))

#define SIPROUND							\
do {									\
	v0 += v1; v1 = ROTL(v1, 13); v1 ^= v0; v0 = ROTL(v0, 32);	\
	v2 += v3; v3 = ROTL(v3, 16); v3 ^= v2;				\
	v0 += v3; v3 = ROTL(v3, 21); v3 ^= v0;				\
	v2 += v1; v1 = ROTL(v1, 17); v1 ^= v2; v2 = ROTL(v2, 32);	\
} while(0)

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

/* SipHash-2-4 with previous output folding, a poor man's streaming interface */
void siphash24fold(unsigned char *out, const unsigned char *in, unsigned long long inlen, const unsigned char *k);

void siphash24fold(unsigned char *out, const unsigned char *in, unsigned long long inlen, const unsigned char *k)
{
	u64 k0 = U8TO64_LE(k);
	u64 k1 = U8TO64_LE(k + 8);
	/* "somepseudorandomlygeneratedbytes" */
	u64 v0 = 0x736f6d6570736575ULL ^ k0;
	u64 v1 = 0x646f72616e646f6dULL ^ k1;
	u64 v2 = 0x6c7967656e657261ULL ^ k0;
	u64 v3 = 0x7465646279746573ULL ^ k1;
	u64 b, m;
	const u8 * const end = in + inlen - (inlen % sizeof(u64));
	const int left = inlen & 7;
	b = ((u64)inlen) << 56;

	// fold in the previous output
	m = U8TO64_LE(out);
	v3 ^= m;
	SIPROUND;
	SIPROUND;
	v0 ^= m;

	// consume full input blocks
	for (; in != end; in += 8) {
		m = U8TO64_LE(in);
		v3 ^= m;
		SIPROUND;
		SIPROUND;
		v0 ^= m;
	}

	// consume the last potentially partial block
	switch (left) {
	case 7: b |= ((u64)in[6]) << 48;
	// FALLTHROUGH
	case 6: b |= ((u64)in[5]) << 40;
	// FALLTHROUGH
	case 5: b |= ((u64)in[4]) << 32;
	// FALLTHROUGH
	case 4: b |= ((u64)in[3]) << 24;
	// FALLTHROUGH
	case 3: b |= ((u64)in[2]) << 16;
	// FALLTHROUGH
	case 2: b |= ((u64)in[1]) <<  8;
	// FALLTHROUGH
	case 1: b |= ((u64)in[0]); break;
	case 0: break;
	}

	// finalize
	v3 ^= b;
	SIPROUND;
	SIPROUND;
	v0 ^= b;
	v2 ^= 0xff;
	SIPROUND;
	SIPROUND;
	SIPROUND;
	SIPROUND;
	b = v0 ^ v1 ^ v2 ^ v3;
	U64TO8_LE(out, b);
}
