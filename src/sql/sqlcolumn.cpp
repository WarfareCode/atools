/*****************************************************************************
* Copyright 2015-2023 Alexander Barthel alex@littlenavmap.org
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

#include "sql/sqlcolumn.h"

#include <QList>
#include <QStringBuilder>

namespace atools {
namespace sql {

QString SqlColumn::getColumnList(const QList<SqlColumn>& columns)
{
  QStringList selectStmt;
  for(const SqlColumn& col : columns)
    selectStmt.append(col.getSelectStmt());

  return selectStmt.join(", ");
}

QString SqlColumn::getSelectStmt() const
{
  return getName() % " as \"" % getDisplayName() % "\"";
}

QString SqlColumn::getColumnList(const QVector<SqlColumn>& columns)
{
  QStringList selectStmt;
  for(const SqlColumn& col : columns)
    selectStmt.append(col.getSelectStmt());

  return selectStmt.join(", ");
}

} // namespace sql
} // namespace atools
