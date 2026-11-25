/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DIRECTOR_RTE_H
#define DIRECTOR_RTE_H

#include "common/array.h"
#include "common/stream.h"

namespace Common {
class SeekableReadStreamEndian;
}

namespace Graphics {
class ManagedSurface;
}

namespace Director {

class Cast;

class RTE0 {
public:
	RTE0(Cast *cast, Common::SeekableReadStreamEndian &stream);

	Cast *_cast;
	Common::Array<byte> data;
};

class RTE1 {
public:
	RTE1(Cast *cast, Common::SeekableReadStreamEndian &stream);

	Cast *_cast;
	Common::Array<byte> data;
};

class RTE2 {
public:
	RTE2(Cast *cast, Common::SeekableReadStreamEndian &stream, uint16 id);
	~RTE2();

	/**
	 * Decodes the coverage bitmap into one coverage value (0 = none .. 15 = full)
	 * and one 0x00RRGGBB colour per pixel, row-major, _width x _height.
	 * The colour starts as foreRGB and changes with every 0x1F code.
	 */
	void decode(uint32 foreRGB, Common::Array<byte> &coverage, Common::Array<uint32> &colors) const;

	Graphics::ManagedSurface *createSurface(uint32 foreColor, uint32 bgColor, const Graphics::PixelFormat &pf, bool renderBg) const;

	/**
	 * Moves one 8-bit channel from dst towards src by k/15 (k = 1..14) with
	 * Director's rounding: the result is rounded towards the lower of the two values.
	 */
	static byte blendChannel(byte src, byte dst, int k) {
		if (src >= dst)
			return dst + (src - dst) * k / 15;
		return src + (dst - src) * (15 - k) / 15;
	}

	Cast *_cast;
	uint16 _id;
	uint16 _width;
	uint16 _height;
	uint32 _bpp;
	Common::Array<byte> _rle;
};

} // End of namespace Director

#endif
