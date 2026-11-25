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

#include "common/config-manager.h"
#include "common/debug.h"
#include "common/file.h"
#include "common/memstream.h"

#include "image/png.h"

#include "director/director.h"
#include "director/cast.h"
#include "graphics/pixelformat.h"
#include "director/rte.h"

namespace Director {

RTE0::RTE0(Cast *cast, Common::SeekableReadStreamEndian &stream) : _cast(cast) {
	if (debugChannelSet(9, kDebugLoading)) {
		debugC(9, kDebugLoading, "RTE0:");
		stream.hexdump(stream.size());
	}
	data.resize(stream.size(), 0);
	if (stream.size())
		stream.read(&data[0], stream.size());
}

RTE1::RTE1(Cast *cast, Common::SeekableReadStreamEndian &stream) : _cast(cast) {
	if (debugChannelSet(9, kDebugLoading)) {
		debugC(9, kDebugLoading, "RTE1:");
		stream.hexdump(stream.size());
	}
	data.resize(stream.size(), 0);
	if (stream.size())
		stream.read(&data[0], stream.size());
}

RTE2::RTE2(Cast *cast, Common::SeekableReadStreamEndian &stream, uint16 id) : _cast(cast), _id(id) {
	if (debugChannelSet(9, kDebugLoading)) {
		debugC(9, kDebugLoading, "RTE2:");
		stream.hexdump(stream.size());
	}
	_width = 0;
	_height = 0;
	_bpp = 0;
	if (!stream.size())
		return;

	_width = stream.readUint16BE();
	_height = stream.readUint16BE();
	_bpp = stream.readUint16BE();
	int rleCount = stream.size() - stream.pos();
	_rle.resize(rleCount, 0x00);
	stream.read(_rle.data(), rleCount);
	debugC(5, kDebugLoading, "RTE2: _width: %d, _height: %d, _bpp: %d, _rle: %d bytes", _width, _height, _bpp, rleCount);
}

void RTE2::decode(uint32 foreRGB, Common::Array<byte> &coverage, Common::Array<uint32> &colors) const {
	coverage.clear();
	colors.clear();
	coverage.resize(_width * _height, 0);
	colors.resize(_width * _height, foreRGB);

	// Each row is a stream of byte codes ending with 0x00 0x00:
	//   0x1F r g b  set the current colour (it carries over into the next rows)
	//   0x00 n      n transparent pixels
	//   0x0F n      n fully covered pixels
	//   c (other)   one pixel with coverage c
	// A row may describe fewer pixels than the width; pixels past the width
	// are ignored, but the row is always read up to its end code.
	uint32 color = foreRGB;
	uint pos = 0;
	const uint size = _rle.size();
	for (int y = 0; y < _height && pos < size; y++) {
		int x = 0;
		while (pos < size) {
			byte code = _rle[pos++];
			if (code == 0x1f) {
				if (pos + 3 > size)
					return;
				color = (_rle[pos] << 16) | (_rle[pos + 1] << 8) | _rle[pos + 2];
				pos += 3;
				continue;
			}
			if (code == 0x00 || code == 0x0f) {
				if (pos >= size)
					return;
				byte count = _rle[pos++];
				if (code == 0x00 && count == 0x00)
					break;	// end of row
				for (int j = 0; j < count; j++, x++) {
					if (x < _width) {
						coverage[y * _width + x] = code;
						colors[y * _width + x] = color;
					}
				}
			} else {
				if (x < _width) {
					coverage[y * _width + x] = MIN<byte>(code, 15);
					colors[y * _width + x] = color;
				}
				x++;
			}
		}
	}
}

Graphics::ManagedSurface *RTE2::createSurface(uint32 foreColor, uint32 bgColor, const Graphics::PixelFormat &pf, bool renderBg) const {
	if (_rle.empty())
		return nullptr;

	byte r, g, b;
	pf.colorToRGB(foreColor, r, g, b);
	Common::Array<byte> coverage;
	Common::Array<uint32> colors;
	decode((r << 16) | (g << 8) | b, coverage, colors);

	byte bgR = 0, bgG = 0, bgB = 0;
	pf.colorToRGB(bgColor, bgR, bgG, bgB);

	// Without a background, the alpha is the coverage (0..15 scaled to 0..255).
	// With one, each pixel is blended into the background exactly as Director does.
	Graphics::Surface surface;
	surface.create((int16)_width, (int16)_height, pf);
	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			int k = coverage[y * _width + x];
			uint32 c = colors[y * _width + x];
			byte cr = (c >> 16) & 0xff, cg = (c >> 8) & 0xff, cb = c & 0xff;
			uint32 out;
			if (!renderBg) {
				out = k ? pf.ARGBToColor(k * 17, cr, cg, cb) : pf.ARGBToColor(0, 0, 0, 0);
			} else if (k == 0) {
				out = pf.ARGBToColor(0xff, bgR, bgG, bgB);
			} else if (k >= 15) {
				out = pf.ARGBToColor(0xff, cr, cg, cb);
			} else {
				out = pf.ARGBToColor(0xff, blendChannel(cr, bgR, k), blendChannel(cg, bgG, k), blendChannel(cb, bgB, k));
			}
			*(uint32 *)surface.getBasePtr(x, y) = out;
		}
	}
	Graphics::ManagedSurface *result = new Graphics::ManagedSurface();
	result->create((int16)_width, (int16)_height, pf);

	result->copyFrom(surface);

	surface.free();

	if (ConfMan.getBool("dump_scripts")) {

		Common::String prepend = _cast->getMacName();
		Common::String filename = Common::String::format("./dumps/%s-%s%s-%d.png",
				encodePathForDump(prepend).c_str(), "RTE2", (renderBg ? "-bg" : ""), _id);
		Common::DumpFile bitmapFile;

		warning("RTE2::createSurface(): Dumping RTE2 to '%s'", filename.c_str());

		bitmapFile.open(Common::Path(filename), true);
		Image::writePNG(bitmapFile, *result->surfacePtr(), nullptr);

		bitmapFile.close();
	}

	return result;
}

RTE2::~RTE2() {
}

} // End of namespace Director
