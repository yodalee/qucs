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
#include <QStatusBar>
#include <QPlainTextEdit>

#include "textwindow.h"
#include "textdoc.h"

TextWindow::TextWindow(QWidget *parent) :
  QMainWindow(parent)
{
  //set up UI, blah blah blah
  DocumentTab = new QTabWidget(this);
  setCentralWidget(DocumentTab);
  DocumentTab->setTabsClosable(true);

#ifdef HAVE_QTABWIDGET_SETMOVABLE
  // make tabs draggable if supported
  DocumentTab->setMovable (true);
#endif
}

TextWindow::~TextWindow()
{
}

void TextWindow::slotFileNew()
{
  statusBar()->message(tr("Creating new textwidget..."));

  QPlainTextEdit *doc = new QPlainTextEdit(this);
  DocumentTab->addTab(doc, "");

  statusBar()->message(tr("Ready."));
}

void TextWindow::slotFileOpen()
{
}

void TextWindow::open(const QString &filename)
{
}
