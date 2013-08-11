/***************************************************************************
                                ground.cpp
                               ------------
    begin                : Sat Aug 23 2003
    copyright            : (C) 2003 by Michael Margraf
    email                : michael.margraf@alumni.tu-berlin.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ground.h"
#include "node.h"

#include <QPainter>

Ground::Ground()
{
  ElemType = isComponent;   // both analog and digital
  Description = QObject::tr("ground (reference potential)");

  Lines.append(new Line(  0,  0,  0, 10,QPen(Qt::darkBlue,2)));
  Lines.append(new Line(-11, 10, 11, 10,QPen(Qt::darkBlue,3)));
  Lines.append(new Line( -7, 16,  7, 16,QPen(Qt::darkBlue,3)));
  Lines.append(new Line( -3, 22,  3, 22,QPen(Qt::darkBlue,3)));

  Ports.append(new Port(  0,  0));

  x1 = -12; y1 =  0;
  x2 =  12; y2 = 25;

  tx = 0;
  ty = 0;
  Model = "GND";
  Name  = "";
  
  setFlags(ItemIsSelectable|ItemIsMovable);
}

Ground::~Ground()
{
}

Component* Ground::newOne()
{
  return new Ground();
}

// -------------------------------------------------------
Element* Ground::info(QString& Name, char* &BitmapFile, bool getNewOne)
{
  Name = QObject::tr("Ground");
  BitmapFile = (char *) "gnd";

  if(getNewOne)  return new Ground();
  return 0;
}

QRectF Ground::boundingRect() const
{
  // return outer bounds of item as a rectangle
  return *(new QRectF(x1, y1, x2-x1, y2-y1));
}

void Ground::paint(QPainter *painter, const QStyleOptionGraphicsItem *item, QWidget *widget)
{
  Q_UNUSED(item);
  Q_UNUSED(widget);
  
  // loop to paint component lines
  foreach (Line *l, Lines) {
    painter->setPen(l->style);
    painter->drawLine(l->x1, l->y1, l->x2, l->y2);
  }
    
  // visualize boundingRect
  painter->setPen(QPen(Qt::red,1));
  painter->drawRect(boundingRect());
}

// -------------------------------------------------------
QString Ground::netlist()
{
  return QString("");
}
