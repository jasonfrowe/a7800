// AstroCart math unit, version 2: the reference model (docs/MATHUNIT.md).
// Version 1 ($0430-$043F: multiply, divide, square root, accumulator, random)
// plus the trig block ($0420-$042F: rotate a vector, angle and length of a
// vector).
// Plain C++11, no dependencies. The a7800 emulator fork carries an identical
// copy as src/devices/bus/a7800/astromath.h.
#ifndef ASTROMATH_H
#define ASTROMATH_H
#include <cstdint>

struct astromath
{
	uint16_t a, b, d;
	uint32_t n, acc, x;
	uint8_t  ctrl;
	uint16_t tx, ty, tt;                          // trig block: X, Y, T

	astromath() { reset(); }
	void reset() { a = b = d = 0; n = acc = 0; ctrl = 0; x = 1; tx = ty = tt = 0; }

	bool is_signed() const { return ctrl & 1; }

	uint32_t p() const
	{
		if (is_signed())
			return uint32_t(int32_t(int16_t(a)) * int32_t(int16_t(b)));
		return uint32_t(a) * uint32_t(b);
	}

	uint32_t q() const
	{
		if (d == 0) return 0xffffffffu;
		if (!is_signed()) return n / d;
		int64_t nn = int32_t(n), dd = int16_t(d);
		return uint32_t(nn / dd);                 // toward zero; $80000000 / -1 wraps
	}

	uint16_t r() const
	{
		if (d == 0) return uint16_t(n);
		if (!is_signed()) return uint16_t(n % d);
		int64_t nn = int32_t(n), dd = int16_t(d);
		return uint16_t(nn % dd);                 // sign of N
	}

	uint16_t s() const                            // floor(sqrt(N)), N unsigned
	{
		uint32_t rem = 0, root = 0, v = n;
		for (int i = 0; i < 16; i++)
		{
			rem = (rem << 2) | (v >> 30);
			v <<= 2;
			root <<= 1;
			uint32_t t = (root << 1) | 1;
			if (rem >= t) { rem -= t; root |= 1; }
		}
		return uint16_t(root);
	}

	// ---- trig block: CORDIC, 18 steps, 4 guard bits on the vector and 5 on
	// the angle. The arithmetic below IS the definition (the results are within
	// about 1.5 of the exact values, not correctly rounded): the hardware does
	// the same steps. Right shifts of negative numbers are arithmetic. ----
	enum { TRIG_STEPS = 18, TRIG_KINV = 79594, TRIG_KINV16 = 39797 };   // 2^17 and 2^16 / (the CORDIC gain 1.646760)
	static int32_t trig_atan(int i)               // atan(2^-i), 2^21 = a full turn
	{
		static const int32_t t[TRIG_STEPS] = { 262144, 154753, 81767, 41506, 20834, 10427, 5215, 2608, 1304,
		                                       652, 326, 163, 81, 41, 20, 10, 5, 3 };
		return t[i];
	}
	static int32_t trig_sra(int64_t v, int n) { return int32_t(v >> n); }
	static uint16_t trig_sat(int32_t v)           // clamp to 16 bits signed
	{
		return uint16_t(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
	}

	// (X, Y) turned by T: XR = X cos T - Y sin T, YR = X sin T + Y cos T
	void rotate(uint16_t &xr, uint16_t &yr) const
	{
		unsigned q = ((tt + 0x2000u) >> 14) & 3;                  // the nearest quarter turn: exact,
		int32_t xi = (q & 1) ? int16_t(ty) : int16_t(tx);         // a swap and changes of sign
		int32_t yi = (q & 1) ? int16_t(tx) : int16_t(ty);
		bool nx = (q == 1 || q == 2), ny = (q == 2 || q == 3);
		int32_t rem = int16_t(uint16_t(tt - (q << 14)));          // what is left: -$2000..$1FFF
		int32_t cx, cy;
		if (rem == 0)
		{   // nothing left: x 16 (the guard bits), no steps
			cx = (nx ? -xi : xi) * 16;
			cy = (ny ? -yi : yi) * 16;
		}
		else
		{
			cx = trig_sra(int64_t(xi) * (nx ? -int32_t(TRIG_KINV16) : int32_t(TRIG_KINV16)), 12);   // x 16 / gain
			cy = trig_sra(int64_t(yi) * (ny ? -int32_t(TRIG_KINV16) : int32_t(TRIG_KINV16)), 12);
			int32_t z = rem * 32;
			for (int i = 0; i < TRIG_STEPS; i++)
			{
				int32_t dx = cy >> i, dy = cx >> i;
				if (z >= 0) { cx -= dx; cy += dy; z -= trig_atan(i); }
				else        { cx += dx; cy -= dy; z += trig_atan(i); }
			}
		}
		xr = trig_sat((cx + 8) >> 4);                             // round off the guard bits, clamp
		yr = trig_sat((cy + 8) >> 4);
	}

	// ANG = the direction of (X, Y) ($10000 = a full turn), LEN = its length
	void vector(uint16_t &ang, uint16_t &len) const
	{
		int32_t cx = int16_t(tx), cy = int16_t(ty);
		if (cx == 0 && cy == 0) { ang = 0; len = 0; return; }
		bool neg = cx < 0;                                        // left half: turn it half way round
		if (neg) { cx = -cx; cy = -cy; }
		cx *= 8; cy *= 8;
		int n = 0;                                                // scale small vectors up
		while (cx >= -0x40000 && cx < 0x40000 && cy >= -0x40000 && cy < 0x40000) { cx *= 2; cy *= 2; n++; }
		int32_t z = 0;
		for (int i = 0; i < TRIG_STEPS; i++)
		{
			int32_t dx = cy >> i, dy = cx >> i;
			if (cy < 0) { cx -= dx; cy += dy; z -= trig_atan(i); }
			else        { cx += dx; cy -= dy; z += trig_atan(i); }
		}
		ang = uint16_t((neg ? 0x8000 : 0) + ((z + 16) >> 5));
		int64_t p = int64_t(cx >> 3) * TRIG_KINV;                 // length x 2^(17+n)
		int32_t l = int32_t(p >> 14) >> n;                        // length x 8
		len = uint16_t((l + 4) >> 3);
	}

	void trig_write(unsigned offset, uint8_t v)
	{
		switch (offset & 15)
		{
		case 0x0: tx = (tx & 0xff00) | v; break;
		case 0x1: tx = (tx & 0x00ff) | (v << 8); break;
		case 0x2: ty = (ty & 0xff00) | v; break;
		case 0x3: ty = (ty & 0x00ff) | (v << 8); break;
		case 0x4: tt = (tt & 0xff00) | v; break;
		case 0x5: tt = (tt & 0x00ff) | (v << 8); break;
		default: break;
		}
	}

	uint8_t trig_read(unsigned offset) const
	{
		uint16_t xr, yr, ang, len;
		switch (offset & 15)
		{
		case 0x0: rotate(xr, yr); return uint8_t(xr);
		case 0x1: rotate(xr, yr); return uint8_t(xr >> 8);
		case 0x2: rotate(xr, yr); return uint8_t(yr);
		case 0x3: rotate(xr, yr); return uint8_t(yr >> 8);
		case 0x4: vector(ang, len); return uint8_t(ang);
		case 0x5: vector(ang, len); return uint8_t(ang >> 8);
		case 0x6: vector(ang, len); return uint8_t(len);
		case 0x7: vector(ang, len); return uint8_t(len >> 8);
		case 0xf: return 0xa2;                    // version: the trig block is here
		default:  return 0x00;
		}
	}

	// offset 0-31 = $0420-$043F
	void write(unsigned offset, uint8_t v)
	{
		if (!(offset & 16)) { trig_write(offset, v); return; }
		switch (offset & 15)
		{
		case 0x0: a = (a & 0xff00) | v; break;
		case 0x1: a = (a & 0x00ff) | (v << 8); break;
		case 0x2: b = (b & 0xff00) | v; break;
		case 0x3: b = (b & 0x00ff) | (v << 8); break;
		case 0x4: n = (n & 0xffffff00u) | v; break;
		case 0x5: n = (n & 0xffff00ffu) | (uint32_t(v) << 8); break;
		case 0x6: n = (n & 0xff00ffffu) | (uint32_t(v) << 16); break;
		case 0x7: n = (n & 0x00ffffffu) | (uint32_t(v) << 24); break;
		case 0x8: d = (d & 0xff00) | v; break;
		case 0x9: d = (d & 0x00ff) | (v << 8); break;
		case 0xa: ctrl = v & 1; break;
		case 0xb:
			if (v == 1) acc = 0;
			else if (v == 2) acc += p();
			else if (v == 3) acc -= p();
			else if (v == 4) n = acc;
			break;
		case 0xe:
			x = (x << 8) | v;
			if (x == 0) x = 1;
			break;
		default: break;
		}
	}

	// A read of offset 30 ($043E) steps the random generator: call once per bus read.
	uint8_t read(unsigned offset)
	{
		if (!(offset & 16)) return trig_read(offset);
		switch (offset & 15)
		{
		case 0x0: return uint8_t(p());
		case 0x1: return uint8_t(p() >> 8);
		case 0x2: return uint8_t(p() >> 16);
		case 0x3: return uint8_t(p() >> 24);
		case 0x4: return uint8_t(q());
		case 0x5: return uint8_t(q() >> 8);
		case 0x6: return uint8_t(q() >> 16);
		case 0x7: return uint8_t(q() >> 24);
		case 0x8: return uint8_t(r());
		case 0x9: return uint8_t(r() >> 8);
		case 0xa: return uint8_t(s());
		case 0xb: return uint8_t(s() >> 8);
		case 0xc: return ctrl;
		case 0xd: return d == 0 ? 0x01 : 0x00;
		case 0xe:
			x ^= x << 13; x ^= x >> 17; x ^= x << 5;
			return uint8_t(x);
		default: return 0xa1;
		}
	}
};
#endif
