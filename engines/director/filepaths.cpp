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

#include "common/str.h"

#include "director/director.h"
#include "director/filepaths.h"

namespace Director {

Common::String getFileName(Common::String path) {
	while (path.contains(g_director->_dirSeparator)) {
		int pos = path.find(g_director->_dirSeparator);
		path = Common::String(&path.c_str()[pos + 1]);
	}
	return path;
}

Common::String getPath(Common::String path) {
	return path;
}

Common::String convertPath(Common::String path) {
	return path;
}

bool testPath(Common::String path) {
	return true;
};


// originally named getPath. NA: what does strrchr do?
Common::String getOriginalPath(Common::String path, Common::String cwd) {
	const char *s;
	if ((s = strrchr(path.c_str(), g_director->_dirSeparator))) {
		return Common::String(path.c_str(), s + 1);
	}

	return cwd; // The path is not altered
}

} // End of namespace Director