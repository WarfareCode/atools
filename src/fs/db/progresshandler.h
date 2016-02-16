/*****************************************************************************
* Copyright 2015-2016 Alexander Barthel albar965@mailbox.org
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
*****************************************************************************/

#ifndef PROGRESSHANDLER_H
#define PROGRESSHANDLER_H

#include "fs/bglreaderprogressinfo.h"

#include <functional>

namespace atools {
namespace fs {
class BglReaderOptions;

namespace scenery {
class SceneryArea;
}
namespace db {

class ProgressHandler
{
public:
  ProgressHandler(const atools::fs::BglReaderOptions *options);

  bool reportProgress(const atools::fs::scenery::SceneryArea *sceneryArea, int current = -1);
  bool reportProgress(const QString& bglFilepath, int current = -1);
  bool reportProgressOther(const QString& otherAction, int current = -1);

  void setTotal(int total);

private:
  bool defaultHandler(const atools::fs::BglReaderProgressInfo& inf);

  std::function<bool(const atools::fs::BglReaderProgressInfo&)> handler;

  atools::fs::BglReaderProgressInfo info;

  bool call();

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // PROGRESSHANDLER_H
