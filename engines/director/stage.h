/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef DIRECTOR_STAGE_H
#define DIRECTOR_STAGE_H

#include "graphics/macgui/macwindow.h"
#include "director/lingo/lingo-object.h"

namespace Common {
class Error;
}

namespace Graphics {
class ManagedSurface;
class MacWindow;
class MacWindowManager;
}

namespace Director {

const int SCALE_THRESHOLD = 0x100;

class Channel;
struct MacShape;

struct TransParams {
	TransitionType type;
	uint frame;
	uint duration;
	uint chunkSize;
	uint area;

	int steps;
	int stepDuration;

	int xStepSize;
	int yStepSize;

	int xpos, ypos;

	int stripSize;

	TransParams() {
		type = kTransNone;
		frame = 0;
		duration = 250;
		chunkSize = 1;
		area = 0;
		steps = 0;
		stepDuration = 0;
		stripSize = 0;

		xStepSize = yStepSize = 0;
		xpos = ypos = 0;
	}

	TransParams(uint16 d, uint16 a, uint16 c, TransitionType t) :
			duration(d), area(a), chunkSize(c), type(t) {
		frame = 0;
		steps = 0;
		stepDuration = 0;
		stripSize = 0;

		xStepSize = yStepSize = 0;
		xpos = ypos = 0;
	}
};

class Stage : public Graphics::MacWindow, public Object<Stage> {
 public:
	Stage(int id, bool scrollable, bool resizable, bool editable, Graphics::MacWindowManager *wm, DirectorEngine *vm);
	~Stage();

	bool render(bool forceRedraw = false, Graphics::ManagedSurface *blitTo = nullptr);
	bool needsAppliedColor(DirectorPlotData *pd);

	void invertChannel(Channel *channel);
	void setStageColor(uint stageColor);
	void addDirtyRect(const Common::Rect &r);
	void mergeDirtyRects();
	void reset();

	// transitions.cpp
	void exitTransition(Graphics::ManagedSurface *nextFrame, Common::Rect clipRect);
	void playTransition(uint16 transDuration, uint8 transArea, uint8 transChunkSize, TransitionType transType, uint frame);
	void initTransParams(TransParams &t, Common::Rect &clipRect);
	void dissolveTrans(TransParams &t, Common::Rect &clipRect, Graphics::ManagedSurface *tmpSurface);
	void dissolvePatternsTrans(TransParams &t, Common::Rect &clipRect, Graphics::ManagedSurface *tmpSurface);
	void transMultiPass(TransParams &t, Common::Rect &clipRect, Graphics::ManagedSurface *tmpSurface);
	void transZoom(TransParams &t, Common::Rect &clipRect, Graphics::ManagedSurface *tmpSurface);

	Common::Point getMousePos();

	DirectorEngine *getVM() const { return _vm; }
	Archive *getMainArchive() const { return _mainArchive; }
	Movie *getCurrentMovie() const { return _currentMovie; }
	Common::String getCurrentPath() const { return _currentPath; }

	virtual void setVisible(bool visible, bool silent = false);
	bool setNextMovie(Common::String &movieFilenameRaw);

	bool step();

	// tests.cpp
	Common::HashMap<Common::String, Movie *> *scanMovies(const Common::String &folder);
	void testFontScaling();
	void testFonts();
	void enqueueAllMovies();
	MovieReference getNextMovieFromQueue();
	void runTests();

	// resource.cpp
	Common::Error loadInitialMovie();
	Archive *openMainArchive(const Common::String movie);
	void loadEXE(const Common::String movie);
	void loadEXEv3(Common::SeekableReadStream *stream);
	void loadEXEv4(Common::SeekableReadStream *stream);
	void loadEXEv5(Common::SeekableReadStream *stream);
	void loadEXEv7(Common::SeekableReadStream *stream);
	void loadEXERIFX(Common::SeekableReadStream *stream, uint32 offset);
	void loadMac(const Common::String movie);

	// lingo/lingo-object.cpp
	virtual bool hasProp(const Common::String &propName);
	virtual Datum getProp(const Common::String &propName);
	virtual bool setProp(const Common::String &propName, const Datum &value);
	virtual Datum getField(int field);
	virtual bool setField(int field, const Datum &value);

public:
	Common::List<Common::Rect> _dirtyRects;
	Common::List<Channel *> _dirtyChannels;
	TransParams *_puppetTransition;

	MovieReference _nextMovie;
	Common::List<MovieReference> _movieStack;
	bool _newMovieStarted;

private:
	uint _stageColor;

	DirectorEngine *_vm;
	Archive *_mainArchive;
	Common::MacResManager *_macBinary;
	Movie *_currentMovie;
	Common::String _currentPath;
	Common::StringArray _movieQueue;

private:
	int preprocessColor(DirectorPlotData *p, int src);

	void inkBlitFrom(Channel *channel, Common::Rect destRect, Graphics::ManagedSurface *blitTo = nullptr);
	void inkBlitShape(DirectorPlotData *pd, Common::Rect &srcRect);

	void inkBlitSurface(DirectorPlotData *pd, Common::Rect &srcRect, const Graphics::Surface *mask);
	void inkBlitStretchSurface(DirectorPlotData *pd, Common::Rect &srcRect, const Graphics::Surface *mask);
};

} // end of namespace Director

#endif
