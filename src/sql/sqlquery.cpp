/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
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

#include "sql/sqlquery.h"
#include "atools.h"
#include "sql/sqlexception.h"
#include "sql/sqldatabase.h"

#include "sql/sqlrecord.h"

#include <QSqlError>
#include <QDebug>
#include <QStringBuilder>

namespace atools {

namespace sql {

SqlQuery::SqlQuery(QSqlResult *r)
{
  query = QSqlQuery(r);
  queryString = QString();
  db = new SqlDatabase();
}

SqlQuery::SqlQuery(const QString& queryStr, const SqlDatabase& sqlDb)
{
  query = QSqlQuery(queryStr, sqlDb.getQSqlDatabase());
  queryString = queryStr;
  db = new SqlDatabase(sqlDb);
}

SqlQuery::SqlQuery(const QString& queryStr, const SqlDatabase *sqlDb)
{
  query = QSqlQuery(queryStr, sqlDb->getQSqlDatabase());
  queryString = queryStr;
  db = new SqlDatabase(*sqlDb);
}

SqlQuery::SqlQuery(const SqlDatabase *sqlDb)
{
  query = QSqlQuery(sqlDb->getQSqlDatabase());
  queryString = QString();
  db = new SqlDatabase(*sqlDb);
}

SqlQuery::SqlQuery(const SqlDatabase& sqlDb)
{
  query = QSqlQuery(sqlDb.getQSqlDatabase());
  queryString = QString();
  db = new SqlDatabase(sqlDb);
}

SqlQuery::SqlQuery(const SqlQuery& other)
{
  query = other.query;
  queryString = other.queryString;
  placeholderList = other.placeholderList;
  placeholderSet = other.placeholderSet;
  db = new SqlDatabase(*other.db);

}

SqlQuery::SqlQuery(const QSqlQuery& otherQuery, QString queryStr)
{
  query = otherQuery;
  queryString = queryStr;
  db = new SqlDatabase();
}

SqlQuery& SqlQuery::operator=(const SqlQuery& other)
{
  query = other.query;
  queryString = other.queryString;
  placeholderList = other.placeholderList;
  placeholderSet = other.placeholderSet;

  delete db;
  db = new SqlDatabase(*other.db);

  return *this;
}

SqlQuery::~SqlQuery()
{
  delete db;
}

bool SqlQuery::isValid() const
{
  return query.isValid();
}

bool SqlQuery::isActive() const
{
  return query.isActive();
}

bool SqlQuery::isNull(int field) const
{
  checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");

  if(field >= query.record().count())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Value index " % QString::number(
                         field) % " does not exist in query \"" % queryString %
                       "\"");

  return query.isNull(field);
}

bool SqlQuery::isNull(const QString& name) const
{
  checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");

  if(!query.record().contains(name))
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Value name \"" % name % "\" does not exist in query \"" % queryString % "\"");

  return query.isNull(name);
}

int SqlQuery::at() const
{
  return query.at();
}

QString SqlQuery::lastQuery() const
{
  return query.lastQuery();
}

int SqlQuery::numRowsAffected() const
{
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.numRowsAffected();
}

QSqlError SqlQuery::lastError() const
{
  return query.lastError();
}

bool SqlQuery::isSelect() const
{
  return query.isSelect();
}

int SqlQuery::size() const
{
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.size();
}

bool SqlQuery::isForwardOnly() const
{
  return query.isForwardOnly();
}

SqlRecord SqlQuery::record(bool allowInvalidQuery) const
{
  if(!allowInvalidQuery)
    checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return SqlRecord(query.record(), queryString);
}

void SqlQuery::setForwardOnly(bool forward)
{
  query.setForwardOnly(forward);
}

void SqlQuery::exec(const QString& queryStr)
{
  queryString = queryStr;
  checkError(query.exec(queryStr), QLatin1String(Q_FUNC_INFO) % ": Error executing query");

  if(db->isAutocommit())
    db->commit();
}

QVariant SqlQuery::value(int i) const
{
  checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  QVariant retval = query.value(i);
  if(!retval.isValid())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Value index " % QString::number(
                         i) % " does not exist in query \"" % queryString % "\"");
  return retval;
}

QVariant SqlQuery::value(const QString& name) const
{
  checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  QVariant retval = query.value(name);
  if(!retval.isValid())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Value name \"" % name % "\" does not exist in query \"" % queryString % "\"");
  return retval;
}

bool SqlQuery::hasField(const QString& name) const
{
  checkError(isValid(), QLatin1String(Q_FUNC_INFO) % " on invalid query");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.record().contains(name);
}

void SqlQuery::setNumericalPrecisionPolicy(QSql::NumericalPrecisionPolicy precisionPolicy)
{
  query.setNumericalPrecisionPolicy(precisionPolicy);
}

QSql::NumericalPrecisionPolicy SqlQuery::numericalPrecisionPolicy() const
{
  return query.numericalPrecisionPolicy();
}

bool SqlQuery::seek(int i, bool relative)
{
  checkError(isSelect(), QLatin1String(Q_FUNC_INFO) % " on query which is not a select");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.seek(i, relative);
}

bool SqlQuery::next()
{
  checkError(isSelect(), QLatin1String(Q_FUNC_INFO) % " on query which is not a select");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.next();
}

bool SqlQuery::previous()
{
  checkError(isSelect(), QLatin1String(Q_FUNC_INFO) % " on query which is not a select");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.previous();
}

bool SqlQuery::first()
{
  checkError(isSelect(), QLatin1String(Q_FUNC_INFO) % " on query which is not a select");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.first();
}

bool SqlQuery::last()
{
  checkError(isSelect(), QLatin1String(Q_FUNC_INFO) % " on query which is not a select");
  checkError(isActive(), QLatin1String(Q_FUNC_INFO) % " on inactive query");
  return query.last();
}

void SqlQuery::clear()
{
  query.clear();
}

void SqlQuery::clearBoundValues()
{
  int pos = 0;
  for(const QVariant& value : boundValues())
  {
    if(value.isValid() && !value.isNull())
      query.bindValue(pos, QVariant(value.type()));
    pos++;
  }
}

void SqlQuery::exec()
{
#ifdef DEBUG_SQL_INFORMATION
  qDebug() << Q_FUNC_INFO << queryString;
  qDebug() << Q_FUNC_INFO << boundValuesAsString();
#endif

  checkError(query.exec(), QLatin1String(Q_FUNC_INFO) % ": Error executing query");
  if(db->isAutocommit())
    db->commit();
}

void SqlQuery::execBatch(QSqlQuery::BatchExecutionMode mode)
{
  checkError(query.execBatch(mode), QLatin1String(Q_FUNC_INFO) % ": Error executing query batch");

  if(db->isAutocommit())
    db->commit();
}

void SqlQuery::prepare(const QString& queryStr)
{
  queryString = queryStr;
  checkError(query.prepare(queryStr), QLatin1String(Q_FUNC_INFO) % ": Error executing prepare");

  // Extract named or positional bindings
  placeholderList = extractPlaceholders(queryString);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  placeholderSet = QSet<QString>(placeholderList.begin(), placeholderList.end());
#else
  placeholderSet = placeholderList.toSet();
#endif
}

void SqlQuery::bindValue(const QString& placeholder, const QVariant& val, QSql::ParamType type)
{
  checkPlaceholder(Q_FUNC_INFO, placeholder);
  query.bindValue(placeholder, val, type);
}

void SqlQuery::bindValue(int pos, const QVariant& val, QSql::ParamType type)
{
  checkPos(Q_FUNC_INFO, pos);
  query.bindValue(pos, val, type);
}

void SqlQuery::bindValues(const QVector<std::pair<QString, QVariant> >& bindValues)
{
  for(const std::pair<QString, QVariant>& bind : bindValues)
    bindValue(bind.first, bind.second);
}

void SqlQuery::bindValues(const QVector<std::pair<int, QVariant> >& bindValues)
{
  for(const std::pair<int, QVariant>& bind : bindValues)
    bindValue(bind.first, bind.second);
}

void SqlQuery::addBindValue(const QVariant& val, QSql::ParamType type)
{
  query.addBindValue(val, type);
}

void SqlQuery::bindNullStr(const QString& placeholder)
{
  checkPlaceholder(Q_FUNC_INFO, placeholder);
  query.bindValue(placeholder, QVariant(QVariant::String));
}

void SqlQuery::bindNullStr(int pos)
{
  checkPos(Q_FUNC_INFO, pos);
  query.bindValue(pos, QVariant(QVariant::String));
}

void SqlQuery::bindNullInt(const QString& placeholder)
{
  checkPlaceholder(Q_FUNC_INFO, placeholder);
  query.bindValue(placeholder, QVariant(QVariant::Int));
}

void SqlQuery::bindNullInt(int pos)
{
  checkPos(Q_FUNC_INFO, pos);
  query.bindValue(pos, QVariant(QVariant::Int));
}

void SqlQuery::bindNullFloat(const QString& placeholder)
{
  checkPlaceholder(Q_FUNC_INFO, placeholder);
  query.bindValue(placeholder, QVariant(QVariant::Double));
}

void SqlQuery::bindNullFloat(int pos)
{
  checkPos(Q_FUNC_INFO, pos);
  query.bindValue(pos, QVariant(QVariant::Double));
}

void SqlQuery::bindRecord(const SqlRecord& record, const QString& bindPrefix)
{
  for(int i = 0; i < record.count(); i++)
    bindValue(bindPrefix % record.fieldName(i), record.value(i));
}

void SqlQuery::bindAndExecRecords(const SqlRecordList& records, const QString& bindPrefix)
{
  for(const SqlRecord& record:records)
  {
    bindRecord(record, bindPrefix);
    exec();
    if(numRowsAffected() != 1)
      qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1. Record " << record;

    clearBoundValues();
  }
}

void SqlQuery::bindAndExecRecord(const SqlRecord& record, const QString& bindPrefix)
{
  bindRecord(record, bindPrefix);
  exec();
  if(numRowsAffected() != 1)
    qWarning() << Q_FUNC_INFO << "query.numRowsAffected() != 1. Record " << record;

  clearBoundValues();
}

QVariant SqlQuery::boundValue(const QString& placeholder, bool ignoreInvalid) const
{
  if(!ignoreInvalid)
    checkPlaceholder(Q_FUNC_INFO, placeholder);

  QVariant v = query.boundValue(placeholder);
  if(!ignoreInvalid && !v.isValid())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Bind name \"" % placeholder % "\" does not exist in query \"" % queryString % "\"");
  return v;
}

QVariant SqlQuery::boundValue(int pos, bool ignoreInvalid) const
{
  if(!ignoreInvalid)
    checkPos(Q_FUNC_INFO, pos);
  QVariant v = query.boundValue(pos);

  if(!ignoreInvalid && !v.isValid())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Bind index " % QString::number(
                         pos) % " does not exist in query \"" % queryString % "\"");
  return v;
}

QStringList SqlQuery::extractPlaceholders(const QString& query)
{
  QStringList placeholders;

  QString currentPlaceholder;
  bool placeholder = false, quote = false, hasNamed = false, hasPositional = false;
  int valueIndex = 0;
  for(int i = 0; i < query.size(); i++)
  {
    QChar character = query.at(i);

    // Toggle quote status - placeholders in single quotes are ignored
    if(character == '\'')
      quote = !quote;

    if(!quote && character != '\'')
    {
      if(character == '?')
      {
        // Positional placeholder - add number
        placeholders.append(QString::number(valueIndex));
        hasPositional = true;
        valueIndex++;
      }
      else if(character == ':')
      {
        // Start of a placeholder - add to name and store status
        currentPlaceholder.append(character);
        placeholder = true;
      }
      else if(placeholder && ((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                              (character >= '0' && character <= '9') || character == '_' || character == '-'))
        // Valid character for a placeholder - add to name after colon detected
        currentPlaceholder.append(character);
      else
      {
        // Normal string or end of placeholder
        if(!currentPlaceholder.isEmpty())
        {
          // Append rest and start new
          placeholders.append(currentPlaceholder);
          hasNamed = true;
          currentPlaceholder.clear();
          valueIndex++;
        }

        // not in placeholder status
        placeholder = false;
      }
    }
  }

  // Add rest
  if(!currentPlaceholder.isEmpty())
  {
    placeholders.append(currentPlaceholder);
    hasNamed = true;
  }

  // ? and :placeholder are not allowed in one string
  if(hasPositional && hasNamed)
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": Found named and unnamed bindings in query \"" % query % "\" bind names " %
                       placeholders.join(", "));

  return placeholders;
}

QMap<QString, QVariant> SqlQuery::boundPlaceholderAndValueMap() const
{
  QVariantList values = boundValues();
  if(placeholderList.size() != values.size())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": bindnames.size() != values.size() " %
                       QString::number(placeholderList.size()) % " != " % QString::number(values.size()) %
                       " for query \"" % queryString % "\"");

  QMap<QString, QVariant> retval;
  for(int i = 0; i < placeholderList.size(); i++)
    retval.insert(placeholderList.at(i), values.at(i));

  return retval;
}

QVariantList SqlQuery::boundValues() const
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QVariantList retval = query.boundValues();

#else
  QVariantList retval = query.boundValues().values();

#endif

  if(placeholderList.size() != retval.size())
    throw SqlException(QLatin1String(Q_FUNC_INFO) % ": bindnames.size() != values.size() " %
                       QString::number(placeholderList.size()) % " != " % QString::number(
                         retval.size()) % " for query \"" % queryString % "\"");

  return retval;
}

QString SqlQuery::executedQuery() const
{
  return query.executedQuery();
}

QVariant SqlQuery::lastInsertId() const
{
  return query.lastInsertId();
}

void SqlQuery::finish()
{
  query.finish();
}

bool SqlQuery::nextResult()
{
  return query.nextResult();
}

QString SqlQuery::boundValuesAsString() const
{
  QMap<QString, QVariant> boundValues = boundPlaceholderAndValueMap();

  QStringList values;
  for(auto it = boundValues.constBegin(); it != boundValues.constEnd(); ++it)
    values.append("\"" % it.key() % "\"=\"" % it.value().toString() % "\"");
  return values.join(",");
}

void SqlQuery::checkPlaceholder(const QString& funcInfo, const QString& placeholder) const
{
  if(!placeholderList.contains(placeholder))
    throw SqlException(funcInfo % ": Placeholder \"" % placeholder % "\" not found in query \"" % queryString % "\"");
}

void SqlQuery::checkPos(const QString& funcInfo, int pos) const
{
  if(!atools::inRange(placeholderList, pos))
    throw SqlException(funcInfo % ": Position \"" % QString::number(pos) % "\" not found in query \"" %
                       queryString % "\"");
}

void SqlQuery::checkError(bool retval, const QString& msg) const
{

  if(!retval || query.lastError().isValid())
  {
    if(queryString.isEmpty())
      throw SqlException(query.lastError(), msg);
    else
    {
      QString msg2("Query is \"" % queryString % "\".");

      QString boundValues = boundValuesAsString();
      if(!boundValues.isEmpty())
        msg2 += " Bound values are [" % boundValues % "]";

      throw SqlException(query.lastError(), msg, msg2);
    }
  }
}

} // namespace sql

} // namespace atools
