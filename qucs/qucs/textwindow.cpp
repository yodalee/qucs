/*
 * textWindow.cpp - implementation of text file simulation program
 *
 * Copyright (C) 2014, Yodalee, lc85301@gmail.com
 *
 * This file is part of Qucs
 *
 * Qucs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Qucs.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QString>

#include "textwindow.h"

TextWindow::TextWindow(QWidget *parent) :
  QMainWindow(parent)
{
  DocumentTab = new QTabWidget();
  DocumentTab->setTabPosition(QTabWidget::West);
}

TextWindow::~TextWindow()
{
}

void TextWindow::slotFileNew()
{
}

void TextWindow::slotFileOpen()
{
}

void TextWindow::open(const QString &filename)
{
}
