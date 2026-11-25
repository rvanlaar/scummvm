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

#include "common/stream.h"
#include "graphics/macgui/macwidget.h"
#include "graphics/macgui/macwindowmanager.h"

#include "director/director.h"
#include "director/cast.h"
#include "director/channel.h"
#include "director/images.h"
#include "director/movie.h"
#include "director/picture.h"
#include "director/rte.h"
#include "director/score.h"
#include "director/sprite.h"
#include "director/window.h"
#include "director/castmember/richtext.h"
#include "director/lingo/lingo-the.h"

namespace Director {

RichTextCastMember::RichTextCastMember(Cast *cast, uint16 castId, Common::SeekableReadStreamEndian &stream, uint16 version)
		: CastMember(cast, castId, stream) {

	_pf32 = Graphics::PixelFormat(4, 8, 8, 8, 8, 24, 16, 8, 0);

	// Defaults of a new member (also kept by files that have no rich text data)
	_initialRect = Common::Rect(0, 0, 360, 20);
	_boundingRect = Common::Rect(0, 0, 360, 20);
	_antialiasFlag = 1;
	_cropFlags = 0;
	_scrollPos = 0;
	_antialiasFontSize = 12;
	_displayHeight = 20;
	_foreColor = _pf32.RGBToColor(0, 0, 0);
	_bgColor = _pf32.RGBToColor(0xff, 0xff, 0xff);
	_rteWidth = _rteHeight = 0;
	_hasText = false;

	// Director 5 reads the 34 bytes of rich text data only from file version 0x4BF on
	if (version >= 0x4BF && version < kFileVer1100) {
		if (debugChannelSet(5, kDebugLoading)) {
			debugC(5, kDebugLoading, "RichTextCastMember():");
			stream.hexdump(stream.size());
		}

		_initialRect = Movie::readRect(stream);
		_boundingRect = Movie::readRect(stream);
		_antialiasFlag = stream.readByte();
		_cropFlags = stream.readByte();
		_scrollPos = stream.readUint16BE();
		_antialiasFontSize = stream.readUint16BE();
		// Files before 0x4C1 from a Windows cast library store pageHeight little-endian
		if (version < 0x4C1 && _cast->_platform != Common::kPlatformMacintosh)
			_displayHeight = stream.readUint16LE();
		else
			_displayHeight = stream.readUint16BE();

		uint8 r = 0, g = 0, b = 0;
		stream.readByte(); // skip one byte
		r = stream.readByte();
		g = stream.readByte();
		b = stream.readByte();
		_foreColor = _pf32.RGBToColor(r, g, b);

		r = (stream.readUint16BE() >> 8);
		g = (stream.readUint16BE() >> 8);
		b = (stream.readUint16BE() >> 8);
		_bgColor = _pf32.RGBToColor(r, g, b);

		// A pageHeight below 1 falls back to the height of the second rectangle
		if ((int16)_displayHeight < 1)
			_displayHeight = _boundingRect.bottom - _boundingRect.top;

		debugC(3, kDebugLoading, "  RichTextCastMember(): initialRect: [%s], boundingRect: [%s], antialiasFlag: 0x%02x, cropFlags: 0x%02x, scrollPos: %d, antialiasFontSize: %d, displayHeight: %d",
			_initialRect.toString().c_str(),
			_boundingRect.toString().c_str(),
			_antialiasFlag,
			_cropFlags,
			_scrollPos,
			_antialiasFontSize,
			_displayHeight);
		debugC(3, kDebugLoading, "  RichTextCastMember(): foreColor: 0x%08x, bgColor: 0x%08x", _foreColor, _bgColor);
	} else if (version >= kFileVer1100) {
		warning("STUB: RichTextCastMember: RTE not yet supported for version v%d (%d)", humanVersion(_cast->_version), _cast->_version);
	}

	_type = kCastRichText;
	_picture = nullptr;
	_pictureWithBg = nullptr;
}

RichTextCastMember::RichTextCastMember(Cast *cast, uint16 castId, RichTextCastMember &source)
		: CastMember(cast, castId) {

	_type = kCastRichText;
	_initialRect = source._initialRect;
	_boundingRect = source._boundingRect;
	_pf32 = source._pf32;
	_antialiasFlag = source._antialiasFlag;
	_cropFlags = source._cropFlags;
	_scrollPos = source._scrollPos;
	_antialiasFontSize = source._antialiasFontSize;
	_displayHeight = source._displayHeight;
	_foreColor = source._foreColor;
	_bgColor = source._bgColor;
	_plainText = source._plainText;
	_rteWidth = source._rteWidth;
	_rteHeight = source._rteHeight;
	_coverage = source._coverage;
	_coverageColors = source._coverageColors;
	_hasText = source._hasText;
	_picture = nullptr;
	_pictureWithBg = nullptr;
	if (cast == source._cast)
		_children = source._children;

}

RichTextCastMember::~RichTextCastMember() {
	if (_picture)
		delete _picture;

	if (_pictureWithBg)
		delete _pictureWithBg;
}

void RichTextCastMember::load() {
	if (_loaded)
		return;

	// RichText casts consist of 3 files:
	// RTE0: Editor data, used only by the Authoring Tool
	// RTE1: Plain text data
	// RTE2: Bitmap representation for rendering
	//
	// RTE0 is using Paige editor by Hermes, which was recently
	// open sourced. So, if anyone wants to look into internals,
	// https://github.com/nmatavka/Hermes-Paige/tree/main
	// the pgReadDoc() is the code entry:
	// https://github.com/nmatavka/Hermes-Paige/blob/main/PGSOURCE/PGREAD.C#L767
	uint rte0id = 0;
	uint rte1id = 0;
	uint rte2id = 0;
	for (auto &it : _children) {
		if (it.tag == MKTAG('R', 'T', 'E', '0')) {
			rte0id = it.index;
		} else if (it.tag == MKTAG('R', 'T', 'E', '1')) {
			rte1id = it.index;
		} else if (it.tag == MKTAG('R', 'T', 'E', '2')) {
			rte2id = it.index;
		}
	}

	if (_cast->_loadedRTE0s.contains(rte0id)) {
		// TODO: Copy the formatted text data
		// There doesn't appear to be a way of using it outside of the Director editor.
	} else {
		warning("RichTextCastMember::load(): rte0tid %i isn't loaded", rte0id);
	}
	if (_cast->_loadedRTE1s.contains(rte1id)) {
		const RTE1 *rte1 =  _cast->_loadedRTE1s.getVal(rte1id);
		// The RTE1 text is Mac Roman, also in movies made on Windows
		_hasText = !rte1->data.empty();
		if (_hasText)
			_plainText = Common::U32String((const char *)&rte1->data[0], rte1->data.size(), Common::kMacRoman);
	} else {
		warning("RichTextCastMember::load(): rte1tid %i isn't loaded, no plain text!", rte1id);
	}
	if (_cast->_loadedRTE2s.contains(rte2id)) {
		_picture = new Picture();
		const RTE2 *rte2 = _cast->_loadedRTE2s.getVal(rte2id);
		byte r, g, b;
		_pf32.colorToRGB(_foreColor, r, g, b);
		rte2->decode((r << 16) | (g << 8) | b, _coverage, _coverageColors);
		_rteWidth = rte2->_width;
		_rteHeight = rte2->_height;
		Graphics::ManagedSurface *surface = rte2->createSurface(_foreColor, _bgColor, _pf32, false);
		if (surface) {
			_picture->_surface.copyFrom(surface->rawSurface());
			surface->free();
			delete surface;
		}

		_pictureWithBg = new Picture();
		surface = rte2->createSurface(_foreColor, _bgColor, _pf32, true);
		if (surface) {
			_pictureWithBg->_surface.copyFrom(surface->rawSurface());
			surface->free();
			delete surface;
		}
	} else {
		warning("RichTextCastMember::load(): rte2tid %i isn't loaded, no bitmap text!", rte2id);
	}

	_loaded = true;
}


Graphics::MacWidget *RichTextCastMember::createWidget(Common::Rect &bbox, Channel *channel, SpriteType spriteType) {
	if (!_picture) {
		warning("RichTextCastMember::createWidget: No picture");
		return nullptr;
	}

	// skip creating widget when the bbox is not available, maybe we should create it using initialRect
	if (!bbox.width() || !bbox.height())
		return nullptr;

	// The stage drawing goes through drawToStage() (see Window::inkBlitFrom), which
	// blends with the pixels already on the stage. The widget holds the opaque
	// rendering (background and text, drawn 1:1, not stretched) for everything else.
	Graphics::MacWidget *widget = new Graphics::MacWidget(g_director->getCurrentWindow()->getMacWindow(), bbox.left, bbox.top, bbox.width(), bbox.height(), g_director->_wm, false);
	Common::Rect local(bbox.width(), bbox.height());
	renderInto(channel, local, Common::Point(0, 0), widget->getSurface(), true);

	return widget;
}

void RichTextCastMember::drawToStage(Channel *channel, const Common::Rect &destRect, Graphics::ManagedSurface *dst) {
	Common::Rect bbox = channel->getBbox();
	renderInto(channel, destRect, Common::Point(bbox.left, bbox.top), dst, false);
}

void RichTextCastMember::renderInto(Channel *channel, const Common::Rect &destRect, const Common::Point &origin, Graphics::ManagedSurface *dst, bool forceOpaque) {
	Sprite *sprite = channel->_sprite;
	Graphics::MacWindowManager *wm = g_director->_wm;
	const bool clut = dst->format.bytesPerPixel == 1;

	// The sprite rectangle, in dst coordinates; the bitmap is drawn from its top-left corner
	Common::Rect spriteRect(origin.x, origin.y, origin.x + channel->getBbox().width(), origin.y + channel->getBbox().height());

	// Ink: blend (ink 32, or the sprite's blend flag) and background transparent (ink 36)
	// draw no background; every other ink behaves like copy. The blend level is
	// the score's blend byte >> 4, and it is subtracted from each pixel's coverage.
	const bool blendInk = sprite->_ink == kInkTypeBlend || (sprite->_thickness & kTHasBlend);
	const bool transparent = !forceOpaque && (blendInk || sprite->_ink == kInkTypeBackgndTrans);
	const int level = (!forceOpaque && blendInk) ? (sprite->_blendAmount >> 4) : 0;

	// Layout mode 1 (scrolling) keeps 16 pixels on the right for the scroll bar.
	// TODO: draw the scroll bar itself.
	Common::Rect clip = spriteRect;
	if (_cropFlags == 1)
		clip.right -= 16;
	clip.clip(destRect);
	clip.clip(Common::Rect(dst->w, dst->h));
	if (clip.isEmpty())
		return;

	auto toNative = [&](byte r, byte g, byte b) -> uint32 {
		return clut ? wm->findBestColor(r, g, b) : dst->format.RGBToColor(r, g, b);
	};

	if (!transparent) {
		byte r, g, b;
		_pf32.colorToRGB(_bgColor, r, g, b);
		dst->fillRect(clip, toNative(r, g, b));
	}

	if (!_hasText || _coverage.empty())
		return;

	// The text rectangle is a window into the bitmap. In layout modes 1 and 2 its
	// top moves down by scrollTop and its height is pageHeight. An empty window is
	// the whole bitmap. Drawing starts at bitmap column |left|, row |top|.
	Common::Rect window = _initialRect;
	if (_cropFlags != 0) {
		window.top = _initialRect.top + (int16)_scrollPos;
		window.bottom = window.top + (int16)_displayHeight;
	}
	int winX = 0, winY = 0;
	if (!window.isEmpty()) {
		winX = ABS<int>(window.left);
		winY = ABS<int>(window.top);
	}

	for (int y = clip.top; y < clip.bottom; y++) {
		int sy = winY + (y - spriteRect.top);
		if (sy < 0 || sy >= _rteHeight)
			continue;
		for (int x = clip.left; x < clip.right; x++) {
			int sx = winX + (x - spriteRect.left);
			if (sx < 0 || sx >= _rteWidth)
				continue;
			int k = _coverage[sy * _rteWidth + sx] - level;
			if (k <= 0)
				continue;
			uint32 c = _coverageColors[sy * _rteWidth + sx];
			byte r = (c >> 16) & 0xff, g = (c >> 8) & 0xff, b = c & 0xff;
			if (k < 15) {
				byte dr, dg, db;
				uint32 d = dst->getPixel(x, y);
				if (clut)
					wm->getPaletteEntry(d, dr, dg, db);
				else
					dst->format.colorToRGB(d, dr, dg, db);
				r = RTE2::blendChannel(r, dr, k);
				g = RTE2::blendChannel(g, dg, k);
				b = RTE2::blendChannel(b, db, k);
			}
			dst->setPixel(x, y, toNative(r, g, b));
		}
	}
}

bool RichTextCastMember::hasField(int field) {
	switch (field) {
	case kTheText:
	case kThePageHeight:
	case kTheScrollTop:
		return true;
	default:
		break;
	}
	return CastMember::hasField(field);
}

Datum RichTextCastMember::getField(int field) {
	Datum d;

	switch (field) {
	case kTheText:
		d = Datum(Common::String(_plainText));
		break;
	case kThePageHeight:
		d = Datum((int)(int16)_displayHeight);
		break;
	case kTheScrollTop:
		d = Datum((int)(int16)_scrollPos);
		break;
	default:
		d = CastMember::getField(field);
		break;
	}

	return d;
}

void RichTextCastMember::setField(int field, const Datum &d) {
	switch (field) {
	case kTheText:
		_plainText = Common::U32String(d.asString());
		warning("STUB: RichTextCastMember::setField: text set to \"%s\", but won't rerender!", d.asString().c_str());
		return;
	case kTheScrollTop:
		_scrollPos = (uint16)d.asInt();
		_modified = true;
		return;
	case kThePageHeight:
		warning("RichTextCastMember::setField: pageHeight is read-only");
		return;
	default:
		break;
	}

	CastMember::setField(field, d);
}

Common::String RichTextCastMember::formatInfo() {
	// need to pull the data from the STXT resource before the
	// debug output will be visible
	load();
	Common::String format = formatStringForDump(_plainText.encode());

	return Common::String::format(
		"initialRect: %dx%d@%d,%d, boundingRect: %dx%d@%d,%d, text: \"%s\"",
		_initialRect.width(), _initialRect.height(),
		_initialRect.left, _initialRect.top,
		_boundingRect.width(), _boundingRect.height(),
		_boundingRect.left, _boundingRect.top,
		format.c_str()
	);
}

uint32 RichTextCastMember::getCastDataSize() {
	if (_cast->_version >= kFileVer500 && _cast->_version < kFileVer1100) {
		// 8 bytes (_initialRect) + 8 bytes (_boundingRect)
		// 1 antialiasFlag, 1 cropFlags, 2 scrollPos, 2 antialiasFontSize,
		// 2 displayHeight, 1 skip byte
		// 3 bytes r, g, b (foreground, each a byte)
		// 6 bytes r, g, b (background, each 2 bytes)
		return 34;
	} else {
		warning("RichTextCastMember()::getCastDataSize(): version v%d isn't handled", humanVersion(_cast->_version));
		return 0;
	}
}

void RichTextCastMember::writeCastData(Common::SeekableWriteStream *writeStream) {
	if (_cast->_version >= kFileVer500 && _cast->_version < kFileVer1100) {
		Movie::writeRect(writeStream, _initialRect);
		Movie::writeRect(writeStream, _boundingRect);

		writeStream->writeByte(_antialiasFlag);
		writeStream->writeByte(_cropFlags);
		writeStream->writeUint16BE(_scrollPos);
		writeStream->writeUint16BE(_antialiasFontSize);
		writeStream->writeUint16BE(_displayHeight);
		writeStream->writeByte(0); // skip byte

		uint8 r, g, b;
		_pf32.colorToRGB(_foreColor, r, g, b);
		writeStream->writeByte(r);
		writeStream->writeByte(g);
		writeStream->writeByte(b);

		_pf32.colorToRGB(_bgColor, r, g, b);
		writeStream->writeUint16BE(r << 8);
		writeStream->writeUint16BE(g << 8);
		writeStream->writeUint16BE(b << 8);
	} else {
		warning("RichTextCastMember()::writeCastData(): version v%d isn't handled", humanVersion(_cast->_version));
	}
}

bool RichTextCastMember::canWriteCastData() {
	return _cast->_version >= kFileVer500 && _cast->_version < kFileVer1100;
}

}	// End of namespace Director
