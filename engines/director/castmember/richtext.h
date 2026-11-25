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

#ifndef DIRECTOR_CASTMEMBER_RICHTEXT_H
#define DIRECTOR_CASTMEMBER_RICHTEXT_H

#include "graphics/pixelformat.h"

#include "director/types.h"
#include "director/castmember/castmember.h"

namespace Director {

class RichTextCastMember : public CastMember {
public:
	RichTextCastMember(Cast *cast, uint16 castId, Common::SeekableReadStreamEndian &stream, uint16 version);
	RichTextCastMember(Cast *cast, uint16 castId, RichTextCastMember &source);
	~RichTextCastMember();

	CastMember *duplicate(Cast *cast, uint16 castId) override { return (CastMember *)(new RichTextCastMember(cast, castId, *this)); }

	void load() override;

	Graphics::MacWidget *createWidget(Common::Rect &bbox, Channel *channel, SpriteType spriteType) override;

	bool hasField(int field) override;
	Datum getField(int field) override;
	void setField(int field, const Datum &value) override;

	Common::String getText() { return Common::String(_plainText); }
	uint32 getCastDataSize() override;
	void writeCastData(Common::SeekableWriteStream *writeStream) override;
	bool canWriteCastData() override;

	Common::String formatInfo() override;

	/**
	 * Draws the member for a sprite the way Director 5 does, onto the stage surface
	 * (in the window manager's pixel format), limited to destRect.
	 * All inks behave like copy except 36 (background transparent) and 32 (blend,
	 * or any ink with the sprite's blend flag): those draw no background, and blend
	 * lowers every pixel's coverage by the blend level.
	 */
	void drawToStage(Channel *channel, const Common::Rect &destRect, Graphics::ManagedSurface *dst);

private:
	void renderInto(Channel *channel, const Common::Rect &destRect, const Common::Point &origin, Graphics::ManagedSurface *dst, bool forceOpaque);

public:

	Common::U32String _plainText;
	Graphics::PixelFormat _pf32;
	Picture *_picture;
	Picture *_pictureWithBg;

	// The decoded RTE2 coverage bitmap: coverage 0..15 and 0x00RRGGBB per pixel
	uint16 _rteWidth;
	uint16 _rteHeight;
	Common::Array<byte> _coverage;
	Common::Array<uint32> _coverageColors;
	bool _hasText;              // RTE1 is not empty; Director draws no text otherwise

	// _initialRect             // text rectangle: a window into the RTE2 bitmap
	// _boundingRect            // second rectangle; its height is the pageHeight fallback
	byte _antialiasFlag;        // unknown, 1 in a new member
	byte _cropFlags;            // layout mode: 0 fixed rect, 1 scrolling (scroll bar), 2 fixed height
	uint16 _scrollPos;          // scrollTop
	uint16 _antialiasFontSize;  // unknown, 12 in a new member
	uint16 _displayHeight;      // pageHeight
	uint32 _foreColor;
	uint32 _bgColor;
};

} // End of namespace Director

#endif
