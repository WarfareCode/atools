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

#ifndef ROUTEEDGEWRITER_H
#define ROUTEEDGEWRITER_H

#include <QVariantList>

class QString;

namespace atools {
namespace geo {
class Rect;
class Pos;
}
namespace sql {
class SqlDatabase;
class SqlQuery;
}

namespace fs {
namespace db {

class RouteEdgeWriter
{
public:
  RouteEdgeWriter(atools::sql::SqlDatabase *sqlDb);

  void run();

private:
  bool nearest(atools::sql::SqlQuery& nearestStmt, int fromNodeId, const geo::Pos& pos, const geo::Rect& queryRect,
               int fromRangeMeter, QVariantList& toNodeIds, QVariantList& toNodeTypes, QVariantList& toNodeDistances);

  atools::sql::SqlDatabase *db;
  void bindCoordinatePointInRect(const atools::geo::Rect& rect, atools::sql::SqlQuery *query);

};

} // namespace writer
} // namespace fs
} // namespace atools

#endif // ROUTEEDGEWRITER_H
