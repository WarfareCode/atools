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

#include "gui/itemviewzoomhandler.h"

#include "settings/settings.h"
#include "atools.h"

#include <QDebug>
#include <QAction>
#include <QFont>
#include <QTableView>
#include <QHeaderView>
#include <QTreeWidget>

namespace atools {
namespace gui {

using atools::settings::Settings;

ItemViewZoomHandler::ItemViewZoomHandler(QAbstractItemView *view, QAction *zoomInAction, QAction *zoomOutAction, QAction *zoomDefaultAction,
                                         QString settingsKeyStr, double marginParm)
{
  init(view, zoomInAction, zoomOutAction, zoomDefaultAction, settingsKeyStr, marginParm);
}

ItemViewZoomHandler::ItemViewZoomHandler(QAbstractItemView *view, double marginParam)
{
  init(view, nullptr, nullptr, nullptr, QString(), marginParam);
}

ItemViewZoomHandler::ItemViewZoomHandler(QAbstractItemView *view)
{
  init(view, nullptr, nullptr, nullptr, QString(), 0.);
}

void ItemViewZoomHandler::init(QAbstractItemView *view, QAction *zoomInAction, QAction *zoomOutAction,
                               QAction *zoomDefaultAction, QString settingsKeyStr, double marginParm)
{
  Q_ASSERT(view != nullptr);

  margin = marginParm;
  itemView = view;
  actionZoomIn = zoomInAction;
  actionZoomOut = zoomOutAction;
  actionZoomDefault = zoomDefaultAction;
  settingsKey = settingsKeyStr;

  initTableViewZoom();

  if(actionZoomIn != nullptr)
    connect(actionZoomIn, &QAction::triggered, this, &ItemViewZoomHandler::zoomIn);
  if(actionZoomOut != nullptr)
    connect(actionZoomOut, &QAction::triggered, this, &ItemViewZoomHandler::zoomOut);
  if(actionZoomDefault != nullptr)
    connect(actionZoomDefault, &QAction::triggered, this, &ItemViewZoomHandler::zoomDefault);
}

ItemViewZoomHandler::~ItemViewZoomHandler()
{
  if(actionZoomIn != nullptr)
    disconnect(actionZoomIn, &QAction::triggered, this, &ItemViewZoomHandler::zoomIn);
  if(actionZoomOut != nullptr)
    disconnect(actionZoomOut, &QAction::triggered, this, &ItemViewZoomHandler::zoomOut);
  if(actionZoomDefault != nullptr)
    disconnect(actionZoomDefault, &QAction::triggered, this, &ItemViewZoomHandler::zoomDefault);
}

void ItemViewZoomHandler::fontChanged()
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName();
#endif

  setTableViewFontSize(defaultTableViewFontPointSize);
}

void ItemViewZoomHandler::setTableViewFontSize(double pointSize)
{
  QFont newFont(itemView->font());
  newFont.setPointSizeF(pointSize);

  double newFontHeight = QFontMetricsF(newFont).height();

#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << "pointSize" << pointSize << itemView->objectName() << "new font height" << newFontHeight
           << "point size" << newFont.pointSizeF();
#endif

  itemView->setFont(newFont);

  QTableView *tableView = dynamic_cast<QTableView *>(itemView);
  if(tableView != nullptr)
  {
    // Adjust the cell height - default is too big
    tableView->verticalHeader()->setMinimumSectionSize(atools::roundToInt(newFontHeight + sectionToFontSize + margin * 2.));
    tableView->verticalHeader()->setDefaultSectionSize(atools::roundToInt(newFontHeight + sectionToFontSize + margin * 2.));
  }
}

void ItemViewZoomHandler::initTableViewZoom()
{
  // Adjust cell height to be smaller than default but according to font height
  defaultTableViewFontPointSize = static_cast<float>(itemView->font().pointSizeF());

  // Increase default table font size for mac
#if defined(Q_OS_MACOS)

  QTableView *tableView = dynamic_cast<QTableView *>(itemView);
  if(tableView != nullptr)
    defaultTableViewFontPointSize *= 1.4;
#endif

  double newPointSize = 0.;
  if(!settingsKey.isEmpty())
    newPointSize = Settings::instance().valueDouble(settingsKey, defaultTableViewFontPointSize);
  else
    newPointSize = defaultTableViewFontPointSize;

#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName() << "newPointSize" << newPointSize
           << "defaultTableViewFontPointSize" << defaultTableViewFontPointSize;
#endif

  setTableViewFontSize(newPointSize);
}

void ItemViewZoomHandler::zoomIn()
{
  zoomTableView(1);
}

void ItemViewZoomHandler::zoomOut()
{
  zoomTableView(-1);
}

void ItemViewZoomHandler::zoomDefault()
{
  zoomTableView(0);
}

void ItemViewZoomHandler::zoomPercent(int percent)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName() << percent;
#endif

  double newPointSize = defaultTableViewFontPointSize * percent / 100.;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, itemView->font().pointSizeF());
  enableDisableZoomActions();
}

void ItemViewZoomHandler::zoomTableView(int value)
{
#ifdef DEBUG_INFORMATION
  qDebug() << Q_FUNC_INFO << itemView->objectName() << value;
#endif

  double newPointSize = defaultTableViewFontPointSize;

  if(value != 0)
    newPointSize = itemView->font().pointSizeF() + value;

  setTableViewFontSize(newPointSize);
  if(!settingsKey.isEmpty())
    Settings::instance().setValue(settingsKey, itemView->font().pointSizeF());
  enableDisableZoomActions();
}

void ItemViewZoomHandler::enableDisableZoomActions()
{
  if(actionZoomDefault != nullptr)
    actionZoomDefault->setEnabled(atools::almostNotEqual(itemView->font().pointSizeF(), defaultTableViewFontPointSize));
  if(actionZoomIn != nullptr)
    actionZoomIn->setEnabled(itemView->font().pointSizeF() < maxFontSize);
  if(actionZoomOut != nullptr)
    actionZoomOut->setEnabled(itemView->font().pointSizeF() > minFontSize);
}

} // namespace gui
} // namespace atools
