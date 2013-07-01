/***************************************************************************
                            schematic_element.cpp
                           -----------------------
    begin                : Sat Mar 3 2006
    copyright            : (C) 2006 by Michael Margraf
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
#include <QtGui>
#include <stdlib.h>
#include <limits.h>

#include "schematic.h"
#include "node.h"
#include "wire.h"
#include "diagrams/diagram.h"
#include "paintings/painting.h"
#include "components/component.h"

#include <QList>



/* *******************************************************************
   *****                                                         *****
   *****              Actions handling the nodes                 *****
   *****                                                         *****
   ******************************************************************* */

// Inserts a port into the schematic and connects it to another node if
// the coordinates are identical. The node is returned.
Node* Schematic::insertNode(int x, int y, Element *e)
{
  Node *pn = 0;
  QListIterator<Node *> in(Nodes);
  // check if new node lies upon existing node
  while (in.hasNext()) { // check every node
    pn = in.next();
    if(pn->cx == x) if(pn->cy == y) {
      pn->Connections.append(e);
      return pn;   // return, if node is not new
    }
  }
 
  // no existing node found at this position, create new node
  pn = new Node(x, y);
  Nodes.append(pn);
  pn->Connections.append(e);  // connect schematic node to component node

  // check if the new node lies upon an existing wire
  Wire *pw = 0;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(pw->x1 == x) {
      if(pw->y1 > y) continue;
      if(pw->y2 < y) continue;
    }
    else if(pw->y1 == y) {
      if(pw->x1 > x) continue;
      if(pw->x2 < x) continue;
    }
    else continue;

    // split the wire into two wires
    splitWire(pw, pn);
    return pn;
  }

  return pn;
}

// ---------------------------------------------------
Node* Schematic::selectedNode(int x, int y)
{
  Node *pn;
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) { // test nodes
    pn = in.next();
    if(pn->getSelected(x, y))
      return pn;
  }

  return 0;
}


/* *******************************************************************
   *****                                                         *****
   *****              Actions handling the wires                 *****
   *****                                                         *****
   ******************************************************************* */

// Inserts a port into the schematic and connects it to another node if the
// coordinates are identical. If 0 is returned, no new wire is inserted.
// If 2 is returned, the wire line ended.
int Schematic::insertWireNode1(Wire *w)
{
  Node *pn = 0;
#warning muta
  QListIterator<Node *> in(Nodes);
//  QMutableListIterator<Node *> in(Nodes);
  // check if new node lies upon an existing node
  while (in.hasNext()) { // check every node
    pn = in.next();
    if(pn->cx == w->x1) 
      if(pn->cy == w->y1) 
        break;
  }

  if(pn != 0) {
    pn->Connections.append(w);
    w->Port1 = pn;
    return 2;   // node is not new
  }

  // check if the new node lies upon an existing wire
  Wire *ptr2 = 0;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    ptr2 = iw.next();
    if(ptr2->x1 == w->x1) {
      if(ptr2->y1 > w->y1) continue;
      if(ptr2->y2 < w->y1) continue;

      if(ptr2->isHorizontal() == w->isHorizontal()) { // ptr2-wire is vertical
        if(ptr2->y2 >= w->y2) {
	  delete w;    // new wire lies within an existing wire
	  return 0; }
        else {
	  // one part of the wire lies within an existing wire
	  // the other part not
          if(ptr2->Port2->Connections.size() == 1) {
            w->y1 = ptr2->y1;
            w->Port1 = ptr2->Port1;
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            ptr2->Port1->Connections.removeOne(ptr2); // two -> one wire
            ptr2->Port1->Connections.append(w);
            Nodes.removeOne(ptr2->Port2);
            Wires.removeOne(ptr2);
            return 2;
          }
          else {
            w->y1 = ptr2->y2;
            w->Port1 = ptr2->Port2;
            ptr2->Port2->Connections.append(w);   // shorten new wire
            return 2;
          }
        }
      }
    }
    else if(ptr2->y1 == w->y1) {
      if(ptr2->x1 > w->x1) continue;
      if(ptr2->x2 < w->x1) continue;

      if(ptr2->isHorizontal() == w->isHorizontal()) { // ptr2-wire is horizontal
        if(ptr2->x2 >= w->x2) {
          delete w;   // new wire lies within an existing wire
          return 0;
        }
        else {
	  // one part of the wire lies within an existing wire
	  // the other part not
          if(ptr2->Port2->Connections.size() == 1) {
            w->x1 = ptr2->x1;
            w->Port1 = ptr2->Port1;
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            ptr2->Port1->Connections.removeOne(ptr2); // two -> one wire
            ptr2->Port1->Connections.append(w);
            Nodes.removeOne(ptr2->Port2);
            Wires.removeOne(ptr2);
            return 2;
          }
          else {
            w->x1 = ptr2->x2;
            w->Port1 = ptr2->Port2;
            ptr2->Port2->Connections.append(w);   // shorten new wire
            return 2;
          }
        }
      }
    }
    else continue;

    pn = new Node(w->x1, w->y1);   // create new node
    Nodes.append(pn);
    pn->Connections.append(w);  // connect schematic node to the new wire
    w->Port1 = pn;

    // split the wire into two wires
    splitWire(ptr2, pn);
    return 2;
  }

  pn = new Node(w->x1, w->y1);   // create new node
  Nodes.append(pn);
  pn->Connections.append(w);  // connect schematic node to the new wire
  w->Port1 = pn;
  return 1;
}

// ---------------------------------------------------
// if possible, connect two horizontal wires to one
bool Schematic::connectHWires1(Wire *w)
{
  Wire *pw = 0;
  Node *n = w->Port1;

  QListIterator<Element *> ie(n->Connections);
  ie.toBack();
  //pw = (Wire*)n->Connections.last();  // last connection is the new wire itself
  while (ie.hasPrevious()) {
    pw = (Wire*)ie.previous();
  //for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
    if(pw->Type != isWire) continue;
    if(!pw->isHorizontal()) continue;
    if(pw->x1 < w->x1) {
      if(n->Connections.size() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      else if(n->Label) {
	     w->Label = n->Label;
	     w->Label->pOwner = w;
	     w->Label->Type = isHWireLabel;
	   }
      w->x1 = pw->x1;
      w->Port1 = pw->Port1;      // new wire lengthens an existing one
      Nodes.removeOne(n);
      w->Port1->Connections.removeOne(pw);
      w->Port1->Connections.append(w);
      Wires.removeOne(pw);
      return true;
    }
    if(pw->x2 >= w->x2) {  // new wire lies within an existing one ?
      w->Port1->Connections.removeOne(w); // second node not yet made
      delete w;
      return false;
    }
    if(pw->Port2->Connections.size() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port1->Connections.removeOne(pw);
      Nodes.removeOne(pw->Port2);
      Wires.removeOne(pw);
      return true;
    }
    w->x1 = pw->x2;    // shorten new wire according to an existing one
    w->Port1->Connections.removeOne(w);
    w->Port1 = pw->Port2;
    w->Port1->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// if possible, connect two vertical wires to one
bool Schematic::connectVWires1(Wire *w)
{
  Wire *pw = 0;
  Node *n = w->Port1;

//  pw = (Wire*)n->Connections.last();  // last connection is the new wire itself
//  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
  QListIterator<Element *> ie(n->Connections);
  ie.toBack();
  while (ie.hasPrevious()) {
    pw = (Wire*)ie.previous();  
    if(pw->Type != isWire) continue;
    if(pw->isHorizontal()) continue;
    if(pw->y1 < w->y1) {
      if(n->Connections.size() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      else if(n->Label) {
	     w->Label = n->Label;
	     w->Label->pOwner = w;
	     w->Label->Type = isVWireLabel;
	   }
     w->y1 = pw->y1;
      w->Port1 = pw->Port1;         // new wire lengthens an existing one
      Nodes.removeOne(n);
      w->Port1->Connections.removeOne(pw);
      w->Port1->Connections.append(w);
      Wires.removeOne(pw);
      return true;
    }
    if(pw->y2 >= w->y2) {  // new wire lies complete within an existing one ?
      w->Port1->Connections.removeOne(w); // second node not yet made
      delete w;
      return false;
    }
    if(pw->Port2->Connections.size() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port1->Connections.removeOne(pw);
      Nodes.removeOne(pw->Port2);
      Wires.removeOne(pw);
      return true;
    }
    w->y1 = pw->y2;    // shorten new wire according to an existing one
    w->Port1->Connections.removeOne(w);
    w->Port1 = pw->Port2;
    w->Port1->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// Inserts a port into the schematic and connects it to another node if the
// coordinates are identical. If 0 is returned, no new wire is inserted.
// If 2 is returned, the wire line ended.
int Schematic::insertWireNode2(Wire *w)
{
  Node *pn = 0;
  // check if new node lies upon an existing node
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) { // check every node
    pn = in.next();
    if(pn->cx == w->x2) if(pn->cy == w->y2) break;
  }

  if(pn != 0) {
    pn->Connections.append(w);
    w->Port2 = pn;
    return 2;   // node is not new
  }

  // check if the new node lies upon an existing wire
  Wire *ptr2 = 0;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    ptr2 = iw.next();
    if(ptr2->x1 == w->x2) {
      if(ptr2->y1 > w->y2) continue;
      if(ptr2->y2 < w->y2) continue;

    // (if new wire lies within an existing wire, was already check before)
      if(ptr2->isHorizontal() == w->isHorizontal()) { // ptr2-wire is vertical
          // one part of the wire lies within an existing wire
          // the other part not
          if(ptr2->Port1->Connections.size() == 1) {
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            w->y2 = ptr2->y2;
            w->Port2 = ptr2->Port2;
            ptr2->Port2->Connections.removeOne(ptr2);  // two -> one wire
            ptr2->Port2->Connections.append(w);
            Nodes.removeOne(ptr2->Port1);
            Wires.removeOne(ptr2);
            return 2;
          }
          else {
            w->y2 = ptr2->y1;
            w->Port2 = ptr2->Port1;
            ptr2->Port1->Connections.append(w);   // shorten new wire
            return 2;
          }
      }
    }
    else if(ptr2->y1 == w->y2) {
      if(ptr2->x1 > w->x2) continue;
      if(ptr2->x2 < w->x2) continue;

    // (if new wire lies within an existing wire, was already check before)
      if(ptr2->isHorizontal() == w->isHorizontal()) { // ptr2-wire is horizontal
          // one part of the wire lies within an existing wire
          // the other part not
          if(ptr2->Port1->Connections.size() == 1) {
	    if(ptr2->Label) {
	      w->Label = ptr2->Label;
	      w->Label->pOwner = w;
	    }
            w->x2 = ptr2->x2;
            w->Port2 = ptr2->Port2;
            ptr2->Port2->Connections.removeOne(ptr2);  // two -> one wire
            ptr2->Port2->Connections.append(w);
            Nodes.removeOne(ptr2->Port1);
            Wires.removeOne(ptr2);
            return 2;
          }
          else {
            w->x2 = ptr2->x1;
            w->Port2 = ptr2->Port1;
            ptr2->Port1->Connections.append(w);   // shorten new wire
            return 2;
          }
      }
    }
    else continue;

    pn = new Node(w->x2, w->y2);   // create new node
    Nodes.append(pn);
    pn->Connections.append(w);  // connect schematic node to the new wire
    w->Port2 = pn;

    // split the wire into two wires
    splitWire(ptr2, pn);
    return 2;
  }

  pn = new Node(w->x2, w->y2);   // create new node
  Nodes.append(pn);
  pn->Connections.append(w);  // connect schematic node to the new wire
  w->Port2 = pn;
  return 1;
}

// ---------------------------------------------------
// if possible, connect two horizontal wires to one
bool Schematic::connectHWires2(Wire *w)
{
  Wire *pw = 0;
  Node *n = w->Port2;

#warning why backwards?
//  pw = (Wire*)n->Connections.last(); // last connection is the new wire itself
//  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
  QListIterator<Element *> ie(n->Connections);
  ie.toBack();
  while (ie.hasPrevious()) {
    pw = (Wire*)ie.previous();
    if(pw->Type != isWire) continue;
    if(!pw->isHorizontal()) continue;
    if(pw->x2 > w->x2) {
      if(n->Connections.size() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      w->x2 = pw->x2;
      w->Port2 = pw->Port2;      // new wire lengthens an existing one
      Nodes.removeOne(n);
      w->Port2->Connections.removeOne(pw);
      w->Port2->Connections.append(w);
      Wires.removeOne(pw);
      return true;
    }
    // (if new wire lies complete within an existing one, was already
    // checked before)

    if(pw->Port1->Connections.size() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port2->Connections.removeOne(pw);
      Nodes.removeOne(pw->Port1);
      Wires.removeOne(pw);
      return true;
    }
    w->x2 = pw->x1;    // shorten new wire according to an existing one
    w->Port2->Connections.removeOne(w);
    w->Port2 = pw->Port1;
    w->Port2->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// if possible, connect two vertical wires to one
bool Schematic::connectVWires2(Wire *w)
{
  Wire *pw = 0;
  Node *n = w->Port2;

#warning why backwards?
//  pw = (Wire*)n->Connections.last(); // last connection is the new wire itself
//  for(pw = (Wire*)n->Connections.prev(); pw!=0; pw = (Wire*)n->Connections.prev()) {
  QListIterator<Element *> ie(n->Connections);
  ie.toBack();
  while (ie.hasPrevious()) {
    pw = (Wire*)ie.previous();
    if(pw->Type != isWire) continue;
    if(pw->isHorizontal()) continue;
    if(pw->y2 > w->y2) {
      if(n->Connections.size() != 2) continue;
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      w->y2 = pw->y2;
      w->Port2 = pw->Port2;     // new wire lengthens an existing one
      Nodes.removeOne(n);
      w->Port2->Connections.removeOne(pw);
      w->Port2->Connections.append(w);
      Wires.removeOne(pw);
      return true;
    }
    // (if new wire lies complete within an existing one, was already
    // checked before)

    if(pw->Port1->Connections.size() < 2) {
      // existing wire lies within the new one
      if(pw->Label) {
        w->Label = pw->Label;
	w->Label->pOwner = w;
      }
      pw->Port2->Connections.removeOne(pw);
      Nodes.removeOne(pw->Port1);
      Wires.removeOne(pw);
      return true;
    }
    w->y2 = pw->y1;    // shorten new wire according to an existing one
    w->Port2->Connections.removeOne(w);
    w->Port2 = pw->Port1;
    w->Port2->Connections.append(w);
    return true;
  }

  return true;
}

// ---------------------------------------------------
// Inserts a vertical or horizontal wire into the schematic and connects
// the ports that hit together. Returns whether the beginning and ending
// (the ports of the wire) are connected or not.
int Schematic::insertWire(Wire *w)
{
  int  tmp, con = 0;
  bool ok;

  // change coordinates if necessary (port 1 coordinates must be less
  // port 2 coordinates)
  if(w->x1 > w->x2) { tmp = w->x1; w->x1 = w->x2; w->x2 = tmp; }
  else
  if(w->y1 > w->y2) { tmp = w->y1; w->y1 = w->y2; w->y2 = tmp; }
  else con = 0x100;

  tmp = insertWireNode1(w);
  if(tmp == 0) return 3;  // no new wire and no open connection
  if(tmp > 1) con |= 2;   // insert node and remember if it remains open

  if(w->isHorizontal()) ok = connectHWires1(w);
  else ok = connectVWires1(w);
  if(!ok) return 3;
  
  tmp = insertWireNode2(w);
  if(tmp == 0) return 3;  // no new wire and no open connection
  if(tmp > 1) con |= 1;   // insert node and remember if it remains open

  if(w->isHorizontal()) ok = connectHWires2(w);
  else ok = connectVWires2(w);
  if(!ok) return 3;

  // change node 1 and 2
  if(con > 255) con = ((con >> 1) & 1) | ((con << 1) & 2);

  Wires.append(w);    // add wire to the schematic

  int  n1, n2;
  Wire *pw, *nw;
  Node *pn, *pn2;
  Element *pe;
  // ................................................................
  // Check if the new line covers existing nodes.
  // In order to also check new appearing wires -> use "for"-loop
  QListIterator<Wire *> iw(Wires);
  QListIterator<Node *> in(Nodes);
//  for(pw = Wires->current(); pw != 0; pw = Wires->next()) ?
  while (iw.hasNext()) {
    pw = iw.next();
    while (in.hasNext()) { // check every node
//    for(pn = Nodes->first(); pn != 0; ) {  // check every node
      if(pn->cx == pw->x1) {
        if(pn->cy <= pw->y1) { pn = in.next(); continue; }
        if(pn->cy >= pw->y2) { pn = in.next(); continue; }
      }
      else if(pn->cy == pw->y1) {
        if(pn->cx <= pw->x1) { pn = in.next(); continue; }
        if(pn->cx >= pw->x2) { pn = in.next(); continue; }
      }
      else { pn = in.next(); continue; }

      n1 = 2; n2 = 3;
      pn2 = pn;
      // check all connections of the current node
//      for(pe=pn->Connections.first(); pe!=0; pe=pn->Connections.next()) {
      QListIterator<Element *> ie(pn->Connections);
      while (ie.hasNext()) {
        pe = ie.next();
        if(pe->Type != isWire) continue;
        nw = (Wire*)pe;
	// wire lies within the new ?
	if(pw->isHorizontal() != nw->isHorizontal()) continue;

        pn  = nw->Port1;
        pn2 = nw->Port2;
        n1  = pn->Connections.size();
        n2  = pn2->Connections.size();
        if(n1 == 1) {
          Nodes.removeOne(pn);     // delete node 1 if open
          pn2->Connections.removeOne(nw);   // remove connection
          pn = pn2;
        }

        if(n2 == 1) {
          pn->Connections.removeOne(nw);   // remove connection
          Nodes.removeOne(pn2);     // delete node 2 if open
          pn2 = pn;
        }

        if(pn == pn2) {
	  if(nw->Label) {
	    pw->Label = nw->Label;
	    pw->Label->pOwner = pw;
	  }
          Wires.removeOne(nw);    // delete wire
//          Wires->findRef(pw);      // set back to current wire
        }
        break;
      }
      if(n1 == 1) if(n2 == 1) continue;

      // split wire into two wires
      if((pw->x1 != pn->cx) || (pw->y1 != pn->cy)) {
        nw = new Wire(pw->x1, pw->y1, pn->cx, pn->cy, pw->Port1, pn);
        pn->Connections.append(nw);
        Wires.append(nw);
//        Wires->findRef(pw);
        pw->Port1->Connections.append(nw);
      }
      pw->Port1->Connections.removeOne(pw);
      pw->x1 = pn2->cx;
      pw->y1 = pn2->cy;
      pw->Port1 = pn2;
      pn2->Connections.append(pw);

      pn = in.next(); // FIXME needed?
    }
  }

  if (Wires.indexOf(w))  // if two wire lines with different labels ...
    oneLabel(w->Port1);       // ... are connected, delete one label
  return con | 0x0200;   // sent also end flag
}

// ---------------------------------------------------
// Follows a wire line and selects it.
void Schematic::selectWireLine(Element *pe, Node *pn, bool ctrl)
{
  Node *pn_1st = pn;
  while(pn->Connections.size() == 2) {
    if(pn->Connections.first() == pe)  pe = pn->Connections.last();
    else  pe = pn->Connections.first();

    if(pe->Type != isWire) break;
    if(ctrl) pe->isSelected ^= ctrl;
    else pe->isSelected = true;

    if(((Wire*)pe)->Port1 == pn)  pn = ((Wire*)pe)->Port2;
    else  pn = ((Wire*)pe)->Port1;
    if(pn == pn_1st) break;  // avoid endless loop in wire loops
  }
}

// ---------------------------------------------------
Wire* Schematic::selectedWire(int x, int y)
{
  Wire *pw = 0;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(pw->getSelected(x, y))
      return pw;
  }

  return 0;
}

// ---------------------------------------------------
// Splits the wire "*pw" into two pieces by the node "*pn".
Wire* Schematic::splitWire(Wire *pw, Node *pn)
{
  Wire *newWire = new Wire(pn->cx, pn->cy, pw->x2, pw->y2, pn, pw->Port2);
  newWire->isSelected = pw->isSelected;

  pw->x2 = pn->cx;
  pw->y2 = pn->cy;
  pw->Port2 = pn;

  newWire->Port2->Connections.prepend(newWire);
  pn->Connections.prepend(pw);
  pn->Connections.prepend(newWire);
  newWire->Port2->Connections.removeOne(pw);
  Wires.append(newWire);

  if(pw->Label)
    if((pw->Label->cx > pn->cx) || (pw->Label->cy > pn->cy)) {
      newWire->Label = pw->Label;   // label goes to the new wire
      pw->Label = 0;
      newWire->Label->pOwner = newWire;
    }

  return newWire;
}

// ---------------------------------------------------
// If possible, make one wire out of two wires.
bool Schematic::oneTwoWires(Node *n)
{
  Wire *e3;
  Wire *e1 = (Wire*)n->Connections.first();  // two wires -> one wire
  Wire *e2 = (Wire*)n->Connections.last();

  if(e1->Type == isWire) if(e2->Type == isWire)
    if(e1->isHorizontal() == e2->isHorizontal()) {
      if(e1->x1 == e2->x2) if(e1->y1 == e2->y2) {
        e3 = e1; e1 = e2; e2 = e3;    // e1 must have lesser coordinates
      }
      if(e2->Label) {   // take over the node name label ?
        e1->Label = e2->Label;
	    e1->Label->pOwner = e1;
      }
      else 
         if(n->Label) {
             e1->Label = n->Label;
	         e1->Label->pOwner = e1;
             if(e1->isHorizontal())
               e1->Label->Type = isHWireLabel;
             else
               e1->Label->Type = isVWireLabel;
	   }

      e1->x2 = e2->x2;
      e1->y2 = e2->y2;
      e1->Port2 = e2->Port2;
      Nodes.removeOne(n);    // delete node (is auto delete)
      e1->Port2->Connections.removeOne(e2);
      e1->Port2->Connections.append(e1);
      Wires.removeOne(e2);
      return true;
    }
  return false;
}

// ---------------------------------------------------
// Deletes the wire 'w'.
void Schematic::deleteWire(Wire *w)
{
  if(w->Port1->Connections.size() == 1) {
    if(w->Port1->Label) delete w->Port1->Label;
    Nodes.removeOne(w->Port1);     // delete node 1 if open
  }
  else {
    w->Port1->Connections.removeOne(w);   // remove connection
    if(w->Port1->Connections.size() == 2)
      oneTwoWires(w->Port1);  // two wires -> one wire
  }

  if(w->Port2->Connections.size() == 1) {
    if(w->Port2->Label) delete w->Port2->Label;
    Nodes.removeOne(w->Port2);     // delete node 2 if open
  }
  else {
    w->Port2->Connections.removeOne(w);   // remove connection
    if(w->Port2->Connections.size() == 2)
      oneTwoWires(w->Port2);  // two wires -> one wire
  }

  if(w->Label) {
    delete w->Label;
    w->Label = 0;
  }
  Wires.removeOne(w);
}

// ---------------------------------------------------
int Schematic::copyWires(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  int count = 0;
  Node *pn;
  Wire *pw;
  WireLabel *pl;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(pw->isSelected) {
      if(pw->x1 < x1) x1 = pw->x1;
      if(pw->x2 > x2) x2 = pw->x2;
      if(pw->y1 < y1) y1 = pw->y1;
      if(pw->y2 > y2) y2 = pw->y2;

      count++;
      ElementCache.append(pw);

      // rescue non-selected node labels
      pn = pw->Port1;
      if(pn->Label)
        if(pn->Connections.size() < 2) {
          ElementCache.append(pn->Label);

          // Don't set pn->Label->pOwner=0 , so text position stays unchanged.
          // But remember its wire.
          pn->Label->pOwner = (Node*)pw;
          pn->Label = 0;
        }
      pn = pw->Port2;
      if(pn->Label)
        if(pn->Connections.size() < 2) {
          ElementCache.append(pn->Label);

          // Don't set pn->Label->pOwner=0 , so text position stays unchanged.
          // But remember its wire.
          pn->Label->pOwner = (Node*)pw;
          pn->Label = 0;
        }

      pl = pw->Label;
      pw->Label = 0;
      deleteWire(pw);
      pw->Label = pl;    // restore wire label
//      pw = Wires->current();
    }
  }

  return count;
}


/* *******************************************************************
   *****                                                         *****
   *****                  Actions with markers                   *****
   *****                                                         *****
   ******************************************************************* */

Marker* Schematic::setMarker(int x, int y)
{
  int n;
  // test all diagrams
  Diagram *pd;
  QListIterator<Diagram *> id(Diagrams);
  id.toBack();
  while (id.hasPrevious()) {
    pd = id.previous();
    if(pd->getSelected(x, y)) {
      // test all graphs of the diagram
      Graph *pg;
      for(int i=0; i<pd->Graphs.size(); i++) { 
        pg = pd->Graphs.at(i);
        n  = pg->getSelected(x-pd->cx, pd->cy-y);
        if(n >= 0) {
	      Marker *pm = new Marker(pd, pg, n, x-pd->cx, y-pd->cy);
          pg->Markers.append(pm);
          setChanged(true, true);
          return pm;
        }
      }
    }
  }

  return 0;
}

// ---------------------------------------------------
// Moves the marker pointer left/right on the graph.
void Schematic::markerLeftRight(bool left, QList<Element *> Elements)
{
  Marker *pm;
  bool acted = false;
  //  for(pm = (Marker*)Elements->first(); pm!=0; pm = (Marker*)Elements->next()) {
  QListIterator<Element *> ie(Elements);
  while (ie.hasNext()) {
    pm = (Marker*)ie.next();
    pm->pGraph->Markers.append(pm);
    if(pm->moveLeftRight(left))
      acted = true;
  }

  if(acted)  setChanged(true, true, 'm');
}

// ---------------------------------------------------
// Moves the marker pointer up/down on the more-dimensional graph.
void Schematic::markerUpDown(bool up, QList<Element *> Elements)
{
  Marker *pm;
  bool acted = false;
//  for(pm = (Marker*)Elements->first(); pm!=0; pm = (Marker*)Elements->next()) {
  QListIterator<Element *> ie(Elements);
  while (ie.hasNext()) {
    pm = (Marker*)ie.next();
    pm->pGraph->Markers.append(pm);
    if(pm->moveUpDown(up))
      acted = true;
  }

  if(acted)  setChanged(true, true, 'm');
}


/* *******************************************************************
   *****                                                         *****
   *****               Actions with all elements                 *****
   *****                                                         *****
   ******************************************************************* */

// Selects the element that contains the coordinates x/y.
// Returns the pointer to the element.
// If 'flag' is true, the element can be deselected.
Element* Schematic::selectElement(float fX, float fY, bool flag, int *index)
{
  int n, x = int(fX), y = int(fY);
  Element *pe_1st=0, *pe_sel=0;
  float Corr = textCorr(); // for selecting text

  WireLabel *pl;
  // test all nodes and their labels
  Node *pn = 0;
  QListIterator<Node *> in(Nodes);
  in.toBack();
  while (in.hasPrevious()) {
//  for( = Nodes->last(); pn != 0; pn = Nodes->prev()) {
    pn = in.previous();
    if(!flag)
      if(index)  // only true if called from MouseActions::MPressSelect()
        if(pn->getSelected(x, y))
          return pn;

    pl = pn->Label;
    if(pl) if(pl->getSelected(x, y)) {
      if(flag) { pl->isSelected ^= flag; return pl; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pl;
      }
      if(pe_1st == 0) pe_1st = pl;  // give access to elements lying beneath
      if(pl->isSelected) pe_sel = pl;
    }
  }

  // test all wires and wire labels
  Wire *pw = 0;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(pw->getSelected(x, y)) {
      if(flag) { pw->isSelected ^= flag; return pw; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pw;
      }
      if(pe_1st == 0) pe_1st = pw;  // give access to elements lying beneath
      if(pw->isSelected) pe_sel = pw;
    }
    pl = pw->Label;
    if(pl) if(pl->getSelected(x, y)) {
      if(flag) { pl->isSelected ^= flag; return pl; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pl;
      }
      if(pe_1st == 0) pe_1st = pl;  // give access to elements lying beneath
      if(pl->isSelected) pe_sel = pl;
    }
  }

  // test all components
  Component *pc = 0;
  QListIterator<Component *> ic(Components);
  ic.toBack();
  while (ic.hasPrevious()) {
    pc = ic.previous();
    if(pc->getSelected(x, y)) {
      if(flag) { pc->isSelected ^= flag; return pc; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pc;
      }
      if(pe_1st == 0) pe_1st = pc;  // give access to elements lying beneath
      if(pc->isSelected) pe_sel = pc;
    }
    else {
      n = pc->getTextSelected(x, y, Corr);
      if(n >= 0) {   // was property text clicked ?
        pc->Type = isComponentText;
        if(index)  *index = n;
        return pc;
      }
    }
  }

  Graph *pg;
  Corr = 5.0 / Scale;  // size of line select and area for resizing
  // test all diagrams
  Diagram *pd;
  Marker *pm;
  QListIterator<Diagram *> id(Diagrams);
  id.toBack();
  while (id.hasPrevious()) {
    pd = id.previous();
    for(int i=0; i<pd->Graphs.size(); i++) { 
      pg = pd->Graphs.at(i);
      // test markers of graphs
      for(int j=0; j<pg->Markers.size(); j++ ){
        pm = pg->Markers.at(j);
        if(pm->getSelected(x-pd->cx, y-pd->cy)) {
          if(flag) { pm->isSelected ^= flag; return pm; }
          if(pe_sel) {
            pe_sel->isSelected = false;
            return pm;
          }
          if(pe_1st == 0) pe_1st = pm; // give access to elements beneath
          if(pm->isSelected) pe_sel = pm;
        }
      }
    }

    // resize area clicked ?
    if(pd->isSelected)
      if(pd->resizeTouched(fX, fY, Corr))
        if(pe_1st == 0) {
          pd->Type = isDiagramResize;
          return pd;
        }

    if(pd->getSelected(x, y)) {
      if(pd->Name[0] == 'T') {   // tabular, timing diagram or truth table ?
        if(pd->Name[1] == 'i') {
          if(y > pd->cy) {
            if(x < pd->cx+pd->xAxis.numGraphs) continue;
            pd->Type = isDiagramHScroll;
            return pd;
          }
        }
        else {
          if(x < pd->cx) {      // clicked on scroll bar ?
            pd->Type = isDiagramVScroll;
            return pd;
          }
        }
      }

      // test graphs of diagram
      Graph *pg;
      for(int i=0; i<pd->Graphs.size(); i++) { 
        pg = pd->Graphs.at(i);
        if(pg->getSelected(x-pd->cx, pd->cy-y) >= 0) {
          if(flag) { pg->isSelected ^= flag; return pg; }
          if(pe_sel) {
            pe_sel->isSelected = false;
            return pg;
          }
          if(pe_1st == 0) pe_1st = pg;  // access to elements lying beneath
          if(pg->isSelected) pe_sel = pg;
        }
      }


      if(flag) { pd->isSelected ^= flag; return pd; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pd;
      }
      if(pe_1st == 0) pe_1st = pd;  // give access to elements lying beneath
      if(pd->isSelected) pe_sel = pd;
    }
  }

  // test all paintings 
#warning why back to front? 
  Painting *pp = 0;
  QListIterator<Painting *> ip(Paintings);
  ip.toBack();
  while (ip.hasPrevious()) {
    pp = ip.previous();
    if(pp->isSelected)
      if(pp->resizeTouched(fX, fY, Corr))
        if(pe_1st == 0) {
          pp->Type = isPaintingResize;
          return pp;
        }

    if(pp->getSelected(fX, fY, Corr)) {
      if(flag) { pp->isSelected ^= flag; return pp; }
      if(pe_sel) {
        pe_sel->isSelected = false;
        return pp;
      }
      if(pe_1st == 0) pe_1st = pp;  // give access to elements lying beneath
      if(pp->isSelected) pe_sel = pp;
    }
  }

  return pe_1st;
}

// ---------------------------------------------------
// Deselects all elements except 'e'.
void Schematic::deselectElements(Element *e)
{
  // test all components
  Component *pc;
  QListIterator<Component *> ic(Components);
  while (ic.hasNext()) {
    pc = ic.next();
    if(e != pc)  pc->isSelected = false;
  }

  // test all wires
  Wire *pw;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(e != pw)  pw->isSelected = false;
    if(pw->Label) if(pw->Label != e)  pw->Label->isSelected = false;
  }

  // test all node labels
  Node *pn;
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) {
    pn = in.next();
    if(pn->Label) if(pn->Label != e)  pn->Label->isSelected = false;
  }
  // test all diagrams
  Diagram *pd;
  QListIterator<Diagram *> id(Diagrams);
  while (id.hasNext()) {
    pd = id.next();
    if(e != pd)  pd->isSelected = false;

    // test graphs of diagram
    Graph *pg;
    for(int i=0; i<pd->Graphs.size(); i++) { 
      pg = pd->Graphs.at(i);
      if(e != pg) pg->isSelected = false;
    
      // test markers of graph
      Marker *pm;
      for(int i=0; i<pg->Markers.size(); i++) {
        pm = pg->Markers.at(i);
        if(e != pm) pm->isSelected = false;
      }
    }
  }
  

  // test all paintings
  Painting *pp;
  QListIterator<Painting *> ip(Paintings);
  while (ip.hasNext()) {
    pp = ip.next();
    if(e != pp)  pp->isSelected = false;
  }
}

// ---------------------------------------------------
// Selects elements that lie within the rectangle x1/y1, x2/y2.
int Schematic::selectElements(int x1, int y1, int x2, int y2, bool flag)
{
  int  z=0;   // counts selected elements
  int  cx1, cy1, cx2, cy2;

  // exchange rectangle coordinates to obtain x1 < x2 and y1 < y2
  cx1 = (x1 < x2) ? x1 : x2; cx2 = (x1 > x2) ? x1 : x2;
  cy1 = (y1 < y2) ? y1 : y2; cy2 = (y1 > y2) ? y1 : y2;
  x1 = cx1; x2 = cx2;
  y1 = cy1; y2 = cy2;

  // test all components
  Component *pc;
  QListIterator<Component *> ic(Components);
  while (ic.hasNext()) {
    pc = ic.next();
    pc->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pc->isSelected = true;  z++;
      continue;
    }
    if(pc->isSelected &= flag) z++;
  }


  Wire *pw;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) { // test all wires
    pw = iw.next();
    if(pw->x1 >= x1) if(pw->x2 <= x2) if(pw->y1 >= y1) if(pw->y2 <= y2) {
      pw->isSelected = true;  z++;
      continue;
    }
    if(pw->isSelected &= flag) z++;
  }


  // test all wire labels *********************************
  WireLabel *pl=0;
  iw.toFront();
  while (iw.hasNext()) { // test all wires
    pw = iw.next();
    if(pw->Label) {
      pl = pw->Label;
      if(pl->x1 >= x1) if((pl->x1+pl->x2) <= x2)
        if(pl->y1 >= y1) if((pl->y1+pl->y2) <= y2) {
          pl->isSelected = true;  z++;
          continue;
        }
      if(pl->isSelected &= flag) z++;
    }
  }


  // test all node labels *************************************
  Node *pn;
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) {
    pn = in.next();
    pl = pn->Label;
    if(pl) {
      if(pl->x1 >= x1) if((pl->x1+pl->x2) <= x2)
        if((pl->y1-pl->y2) >= y1) if(pl->y1 <= y2) {
          pl->isSelected = true;  z++;
          continue;
        }
      if(pl->isSelected &= flag) z++;
    }
  }


  // test all diagrams *******************************************
  Diagram *pd;
  Graph *pg;
  Marker *pm;
  QListIterator<Diagram *> id(Diagrams);
  while (id.hasNext()) {
    pd = id.next();
    // test graphs of diagram
    for(int i=0; i<pd->Graphs.size(); i++) {
      pg = pd->Graphs.at(i);
      if(pg->isSelected &= flag) z++;

      // test markers of graph
      for(int j=0; j<pg->Markers.size(); j++) {
        pm = pg->Markers.at(j);
        pm->Bounding(cx1, cy1, cx2, cy2);
        if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
          pm->isSelected = true;  z++;
          continue;
        }
        if(pm->isSelected &= flag) z++;
      }
    }

    // test diagram itself
    pd->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pd->isSelected = true;  z++;
      continue;
    }
    if(pd->isSelected &= flag) z++;
  }

  // test all paintings *******************************************
  Painting *pp;
  QListIterator<Painting *> ip(Paintings);
  while (ip.hasNext()) {
    pp = ip.next();
    pp->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      pp->isSelected = true;  z++;
      continue;
    }
    if(pp->isSelected &= flag) z++;
  }

  return z;
}

// ---------------------------------------------------
// Selects all markers.
void Schematic::selectMarkers()
{
//  for(Diagram *pd = Diagrams->first(); pd != 0; pd = Diagrams->next())
  Diagram *pd;
  Graph *pg;
  Marker *pm;
  QListIterator<Diagram *> id(Diagrams);
  while (id.hasNext()) {
    pd = id.next();
    for(int i=0; i<pd->Graphs.size(); i++) {
      pg = pd->Graphs.at(i);
      for(int j=0; j<pg->Markers.size(); j++) {
        pm = pg->Markers.at(j);
        pm->isSelected = true;
      }
    }
  }
}

// ---------------------------------------------------
// For moving elements: If the moving element is connected to a not
// moving element, insert two wires. If the connected element is already
// a wire, use this wire. Otherwise create new wire.
void Schematic::newMovingWires(QList<Element *> p, Node *pn, int pos)
{
  Element *pe;

  if(pn->State & 8)  // Were new wires already inserted ?
    return;
  pn->State |= 8;

  for (;;) {
    if(pn->State & 16)  // node was already worked on
      break;

    pe = pn->Connections.first();
    if(pe == 0)  return;

    if(pn->Connections.size() > 1)
      break;
    if(pe->Type != isWire)  // is it connected to exactly one wire ?
      break;

    // .................................................
    long  mask = 1, invMask = 3;
    Wire *pw2=0, *pw = (Wire*)pe;

    Node *pn2 = pw->Port1;
    if(pn2 == pn) pn2 = pw->Port2;

    if(pn2->Connections.size() == 2) // two existing wires connected ?
      if((pn2->State & (8+4)) == 0) {
        Element *pe2 = pn2->Connections.first();
        if(pe2 == pe) pe2 = pn2->Connections.last();
        // connected wire connected to exactly one wire ?
        if(pe2->Type == isWire)
          pw2  = (Wire*)pe2;
      }

    // .................................................
    // reuse one wire
    p.insert(pos, pw);
    pw->Port1->Connections.removeOne(pw);   // remove connection 1
    pw->Port1->State |= 16+4;
    pw->Port2->Connections.removeOne(pw);   // remove connection 2
    pw->Port2->State |= 16+4;
    Wires.takeAt(Wires.indexOf(pw));

    if(pw->isHorizontal()) mask = 2;

    if(pw2 == 0) {  // place new wire between component and old wire
      pn = pn2;
      mask ^= 3;
      invMask = 0;
    }

    if(pw->Port1 != pn) {
      pw->Port1->State |= mask;
      pw->Port1 = (Node*)mask;
      pw->Port2->State |= invMask;
      pw->Port2 = (Node*)invMask;  // move port 2 completely
    }
    else {
      pw->Port1->State |= invMask;
      pw->Port1 = (Node*)invMask;
      pw->Port2->State |= mask;
      pw->Port2 = (Node*)mask;
    }

    invMask ^= 3;
    // .................................................
    // create new wire ?
    if(pw2 == 0) {
      if(pw->Port1 != (Node*)mask)
        p.insert(pos,
          new Wire(pw->x2, pw->y2, pw->x2, pw->y2, (Node*)mask, (Node*)invMask));
      else
        p.insert(pos,
          new Wire(pw->x1, pw->y1, pw->x1, pw->y1, (Node*)mask, (Node*)invMask));
      return;
    }


    // .................................................
    // reuse a second wire
    p.insert(pos, pw2);
    pw2->Port1->Connections.removeOne(pw2);   // remove connection 1
    pw2->Port1->State |= 16+4;
    pw2->Port2->Connections.removeOne(pw2);   // remove connection 2
    pw2->Port2->State |= 16+4;
    Wires.takeAt(Wires.indexOf(pw2));

    if(pw2->Port1 != pn2) {
      pw2->Port1 = (Node*)0;
      pw2->Port2->State |= mask;
      pw2->Port2 = (Node*)mask;
    }
    else {
      pw2->Port1->State |= mask;
      pw2->Port1 = (Node*)mask;
      pw2->Port2 = (Node*)0;
    }
    return;
  }

  // only x2 moving
  p.insert(pos, new Wire(pn->cx, pn->cy, pn->cx, pn->cy, (Node*)0, (Node*)1));
  // x1, x2, y2 moving
  p.insert(pos, new Wire(pn->cx, pn->cy, pn->cx, pn->cy, (Node*)1, (Node*)3));
}

// ---------------------------------------------------
// For moving of elements: Copies all selected elements into the
// list 'p' and deletes them from the document.
int Schematic::copySelectedElements(QList<Element *> p)
{
  qDebug() << "Schematic::copySelectedElements  --- BADLY commented out";
  int i, count = 0;
  Port      *pp;
  Component *pc;
  Wire      *pw;
  Diagram   *pd;
  Element   *pe;
  Node      *pn;


  // test all components *********************************
  // Insert components before wires in order to prevent short-cut removal.
  QListIterator<Component *> ic(Components);
  //for(pc = Components->first(); pc != 0; )
  while (ic.hasNext()) {
    pc = ic.next();
    if(pc->isSelected) {
      p.append(pc);
      count++; // selected component count

      // delete all port connections
      QListIterator<Port *> ip(pc->Ports);
      while (ip.hasNext()) {
        pp = ip.next();
        pp->Connection->Connections.removeOne((Element*)pc);
        pp->Connection->State = 4;
      }

      Components.takeAt(Components.indexOf(pc)); // ->take();   // take component out of the document
//      pc = Components->current();
    }
//    else pc = Components->next();
//    else pc = ic.next(); 
  }

  // test all wires and wire labels ***********************
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    if(pw->Label) if(pw->Label->isSelected)
      p.append(pw->Label);

    if(pw->isSelected) {
      p.append(pw);

      pw->Port1->Connections.removeOne(pw);   // remove connection 1
      pw->Port1->State = 4;
      pw->Port2->Connections.removeOne(pw);   // remove connection 2
      pw->Port2->State = 4;
#warning FIXME
      Wires.takeAt(Wires.indexOf(pw));//->take();  
      //pw = Wires->current();
    }
//    else pw = Wires->next();
  }

  // ..............................................
  // Inserts wires, if a connection to a not moving element is found.
  // The order of the "for"-loops is important to guarantee a stable
  // operation: components, new wires, old wires
  
#warning commented out
  
  /*
  pc = (Component*)p->first();
  for(i=0; i<count; i++) {
    for(pp = pc->Ports.first(); pp!=0; pp = pc->Ports.next())
       newMovingWires(p, pp->Connection, count);

    p->findRef(pc);   // back to the real current pointer
    pc = (Component*)p->next();
  }

  for(pe = (Element*)pc; pe != 0; pe = p->next())  // new wires
    if(pe->isSelected)
      break;

  for(pw = (Wire*)pe; pw != 0; pw = (Wire*)p->next())
    if(pw->Type == isWire) {  // not working on labels
      newMovingWires(p, pw->Port1, count);
      newMovingWires(p, pw->Port2, count);
      p->findRef(pw);   // back to the real current pointer
    }
*/

  // ..............................................
  // delete the unused nodes
  
  QMutableListIterator<Node *> in(Nodes); 
#warning mutable?
  while (in.hasNext()) {
//  for(pn = Nodes->first(); pn!=0; ) {
    pn = in.next();
    if(pn->State & 8)
      if(pn->Connections.size() == 2)
        if(oneTwoWires(pn)) {  // if possible, connect two wires to one
          //pn = Nodes->current();
          continue;
        }

    if(pn->Connections.size() == 0) {
      if(pn->Label) {
        pn->Label->Type = isMovingLabel;
        if(pn->State & 1) {
          if(!(pn->State & 2)) pn->Label->Type = isHMovingLabel;
        }
        else if(pn->State & 2) pn->Label->Type = isVMovingLabel;
        p.append(pn->Label);    // do not forget the node labels
      }
//      Nodes->remove();
      in.remove();
//      Nodes.at(Nodes.indexOf(pn));//->remove();
      in.insert(pn);
//      pn = Nodes->current();
      continue;
    }

    pn->State = 0;
    //pn = Nodes->next();
  }

  // test all node labels
  // do this last to avoid double copying
  QListIterator<Node *> in2(Nodes);
  while (in2.hasNext()) {
    pn = in2.next();
    if(pn->Label) if(pn->Label->isSelected)
      p.append(pn->Label);
  }


  // test all paintings **********************************
//  for(Painting *ppa = Paintings->first(); ppa != 0; )
#warning mutable?
  Painting *ppa;
  QListIterator<Painting *> ip(Paintings);
  while (ip.hasNext()) {
    ppa = ip.next();
    if(ppa->isSelected) {
      p.append(ppa);
      Paintings.takeAt(Paintings.indexOf(ppa));
//      ppa = Paintings->current();
    }
//    else ppa = Paintings->next();
  }

  count = 0;  // count markers now
  // test all diagrams **********************************
//  for(pd = Diagrams->first(); pd != 0; )
  QListIterator<Diagram *> id(Diagrams);
  while (id.hasNext()) {
    pd = id.next();
    if(pd->isSelected) {
      p.append(pd);
//      Diagrams->take();
//      pd = Diagrams->current();
    }
    else {
//      for(Graph *pg = pd->Graphs.first(); pg!=0; pg = pd->Graphs.next())
      Graph *pg;
      for(int i=0; i<pd->Graphs.size(); i++) {
        pg = pd->Graphs.at(i);          
        for(Marker *pm = pg->Markers.first(); pm != 0; )
          if(pm->isSelected) {
            count++;
            p.append(pm);
//            pg->Markers.take();
//            pm = pg->Markers.current();
          }
//          else pm = pg->Markers.next();
      }

//      pd = Diagrams->next();
    }
  }

  return count;
}

// ---------------------------------------------------
bool Schematic::copyComps2WiresPaints(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  x1=INT_MAX;
  y1=INT_MAX;
  x2=INT_MIN;
  y2=INT_MIN;
  copyLabels(x1, y1, x2, y2, ElementCache);   // must be first of all !
  copyComponents2(x1, y1, x2, y2, ElementCache);
  copyWires(x1, y1, x2, y2, ElementCache);
  copyPaintings(x1, y1, x2, y2, ElementCache);

  if(y1 == INT_MAX) return false;  // no element selected
  return true;
}

// ---------------------------------------------------
// Used in "aligning()", "distributeHorizontal()", "distributeVertical()".
int Schematic::copyElements(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  int bx1, by1, bx2, by2;
  //Wires->setAutoDelete(false);
  //Components->setAutoDelete(false);

  x1=INT_MAX;
  y1=INT_MAX;
  x2=INT_MIN;
  y2=INT_MIN;
  // take components and wires out of list, check their boundings
  int number = copyComponents(x1, y1, x2, y2, ElementCache);
  number += copyWires(x1, y1, x2, y2, ElementCache);

  //Wires->setAutoDelete(true);
  //Components->setAutoDelete(true);

  // find upper most selected diagram
  Diagram *pd;
  QListIterator<Diagram *> id(Diagrams);
  id.toBack();
  while (id.hasPrevious()) {
    pd = id.previous();
    if(pd->isSelected) {
      pd->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;
      ElementCache.append(pd);
      number++;
    }
  }
  // find upper most selected painting
#warning why backwards?
  Painting *pp;
  QListIterator<Painting *> ip(Paintings);
  ip.toBack();
  while (ip.hasPrevious()) {
    pp = ip.previous();
    if(pp->isSelected) {
      pp->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;
      ElementCache.append(pp);
      number++;
    }
  }

  return number;
}

// ---------------------------------------------------
// Deletes all selected elements.
bool Schematic::deleteElements()
{
  qDebug() << "Schematic::deleteElements --- BADLY COMMENTED OUT";
  bool sel = false;

  Component *pc;// = Components->first();
  QListIterator<Component *> ic(Components);
//FIXME how can it delete mid loop??
#warning this does not look right
  while(ic.hasNext()) {     // all selected component
    pc = ic.next();
    if(pc->isSelected) {
      deleteComp(pc);     //FIXME maybe a QMutableListIterator, to be deleted
//      pc = Components->current();
      sel = true;
    }
//    else pc = Components->next();
  }

  Wire *pw; // = Wires->first();
#warning mutable  
  QListIterator<Wire *> iw(Wires);
  while(iw.hasNext()) {      // all selected wires and their labels
    pw = iw.next();
    if(pw->Label)
      if(pw->Label->isSelected) {
        delete pw->Label;
        pw->Label = 0;
        sel = true;
      }

    if(pw->isSelected) {
      deleteWire(pw);
//      pw = Wires->current();
      sel = true;
    }
//    else pw = Wires->next();
  }

  // all selected labels on nodes ***************************
  Node *pn;
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) {
    pn = in.next();
    if(pn->Label)
      if(pn->Label->isSelected) {
        delete pn->Label;
        pn->Label = 0;
        sel = true;
      }
  }

  Diagram *pd;// = Diagrams->first();
  QListIterator<Diagram *> id(Diagrams);
  while(id.hasNext()) {      // test all diagrams
    pd = id.next();
    if(pd->isSelected) {
//      Diagrams->remove();
//      pd = Diagrams->current();
      sel = true;
    }
    else {
      bool wasGraphDeleted = false;
      // all graphs of diagram
      for(Graph *pg = pd->Graphs.first(); pg != 0; ) {
        // all markers of diagram
        for(Marker *pm = pg->Markers.first(); pm != 0; )
          if(pm->isSelected) {
//            pg->Markers.remove();
//            pm = pg->Markers.current();
            sel = true;
          }
//          else  pm = pg->Markers.next();

        if(pg->isSelected) {
//          pd->Graphs.remove();
//          pg = pd->Graphs.current();
          sel = wasGraphDeleted = true;
        }
//        else  pg = pd->Graphs.next();
      }
      if(wasGraphDeleted)
        pd->recalcGraphData();  // update diagram (resize etc.)

//      pd = Diagrams->next();
    }
  }

#warning mutable?
  Painting *pp;// = Paintings->first();
  QListIterator<Painting *> ip(Paintings);
  while(ip.hasNext()) {    // test all paintings
    pp = ip.next();
    if(pp->isSelected)
      if(pp->Name.at(0) != '.') {  // do not delete "PortSym", "ID_text"
	sel = true;
	Paintings.removeAt(Paintings.indexOf(pp));
//	pp = Paintings->current();
	continue; // it will either way
      }
//    pp = Paintings->next();
  }

  if(sel) {
    sizeOfAll(UsedX1, UsedY1, UsedX2, UsedY2);   // set new document size
    setChanged(sel, true);
  }
  return sel;
}

// ---------------------------------------------------
bool Schematic::aligning(int Mode)
{
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2, *bx=0, *by=0, *ax=0, *ay=0;
  QList<Element *> ElementCache;
  int count = copyElements(x1, y1, x2, y2, ElementCache);
  if(count < 1) return false;


  ax = ay = &x2;  // = 0
  switch(Mode) {
    case 0:  // align top
	bx = &x1;
	by = &by1;
	y2 = 1;
	break;
    case 1:  // align bottom
	bx = &x1;
	y1 = y2;
	by = &by2;
	y2 = 1;
	break;
    case 2:  // align left
	by = &y1;
	bx = &bx1;
	y2 = 1;
	break;
    case 3:  // align right
	by = &y1;
	x1 = x2;
	bx = &bx2;
	y2 = 1;
	break;
    case 4:  // center horizontally
	x1 = (x2+x1) / 2;
	by = &x2;  // = 0
	ax = &bx1;
	bx = &bx2;
	y1 = 0;
	y2 = 2;
	break;
    case 5:  // center vertically
	y1 = (y2+y1) / 2;
	bx = &x2;  // = 0
	ay = &by1;
	by = &by2;
	x1 = 0;
	y2 = 2;
	break;
  }
  x2 = 0;

  Wire *pw;
  Component *pc;
  // re-insert elements
  // Go backwards in order to insert node labels before its component.
//  for( = ElementCache.last(); pe != 0; pe = ElementCache.prev())
  Element *pe;
  QListIterator<Element *> ie(ElementCache);
  ie.toBack();
  while (ie.hasPrevious()) {
    pe = ie.previous();
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
        pc = (Component*)pe;
        pc->Bounding(bx1, by1, bx2, by2);
        pc->setCenter(x1-((*bx)+(*ax))/y2, y1-((*by)+(*ay))/y2, true);
        insertRawComponent(pc);
        break;

      case isWire:
        pw = (Wire*)pe;
        bx1 = pw->x1;
        by1 = pw->y1;
        bx2 = pw->x2;
        by2 = pw->y2;
        pw->setCenter(x1-((*bx)+(*ax))/y2, y1-((*by)+(*ay))/y2, true);
//        if(pw->Label) {  }
        insertWire(pw);
        break;

      case isDiagram:
        // Should the axis label be counted for ? I guess everyone
        // has a different opinion.
//        ((Diagram*)pe)->Bounding(bx1, by1, bx2, by2);

        // Take size without axis label.
        bx1 = ((Diagram*)pe)->cx;
        by2 = ((Diagram*)pe)->cy;
        bx2 = bx1 + ((Diagram*)pe)->x2;
        by1 = by2 - ((Diagram*)pe)->y2;
        ((Diagram*)pe)->setCenter(x1-((*bx)+(*ax))/y2, y1-((*by)+(*ay))/y2, true);
        break;

      case isPainting:
        ((Painting*)pe)->Bounding(bx1, by1, bx2, by2);
        ((Painting*)pe)->setCenter(x1-((*bx)+(*ax))/y2, y1-((*by)+(*ay))/y2, true);
        break;

      case isNodeLabel:
        if(((Element*)(((WireLabel*)pe)->pOwner))->Type & isComponent) {
          pc = (Component*)(((WireLabel*)pe)->pOwner);
          pc->Bounding(bx1, by1, bx2, by2);
        }
        else {
          pw = (Wire*)(((WireLabel*)pe)->pOwner);
          bx1 = pw->x1;  by1 = pw->y1;
          bx2 = pw->x2;  by2 = pw->y2;
        }
        ((WireLabel*)pe)->cx += x1-((*bx)+(*ax))/y2;
        ((WireLabel*)pe)->cy += y1-((*by)+(*ay))/y2;
        insertNodeLabel((WireLabel*)pe);
        break;

      default: ;
    }
  }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}

// ---------------------------------------------------
bool Schematic::distributeHorizontal()
{
  qDebug() << "Schematic::distributeHorizontal  -- BROKEN";
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2;
  QList<Element *> ElementCache;
  int count = copyElements(x1, y1, x2, y2, ElementCache);
  if(count < 1) return false;

  Element *pe;
  WireLabel *pl;
  // Node labels are not counted for, so put them to the end.
  QMutableListIterator<Element *> ie(ElementCache);
  while (ie.hasPrevious()) {
    pe = ie.previous();
    if(pe->Type == isNodeLabel) {
      ie.remove();
      ElementCache.append(pe);
//      ElementCache.removeRef(pe);
    }
  }
/*  for(pe = ElementCache.last(); pe != 0; pe = ElementCache.prev())
    if(pe->Type == isNodeLabel) {
      ElementCache.append(pe);
      ElementCache.removeRef(pe);
    } it was commented out?! was it working?*/

  // using bubble sort to get elements x ordered
  QMutableListIterator<Element *> ies(ElementCache);
  if(count > 1)
    for(int i = count-1; i>0; i--) {
      ies.toFront();
      pe = ies.next();
      for(int j=0; j<i; j++) {
        pe->getCenter(bx1, by1);
        pe = ies.next();
        pe->getCenter(bx2, by2);
        if(bx1 > bx2) {  // change two elements ?
          // current gets previous, step back one position
          ies.setValue(ies.previous());
          // new current gets temp 
          ies.setValue(pe);
          // step forward to initial position
          ies.next(); 
//          pe = ElementCache.next(); //Need another step?
        }
      }
    }

  ElementCache.last()->getCenter(x2, y2);
  ElementCache.first()->getCenter(x1, y1);
  Wire *pw;
  int x = x2;
  int dx=0;
  if(count > 1) dx = (x2-x1)/(count-1);
  // re-insert elements and put them at right position
  // Go backwards in order to insert node labels before its component.
  QListIterator<Element *> ie2(ElementCache);
  ie2.toBack();
  while (ie2.hasPrevious()) {
    pe = ie2.previous();
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
        pe->cx = x;
        insertRawComponent((Component*)pe);
        break;

      case isWire:
        pw = (Wire*)pe;
        if(pw->isHorizontal()) {
          x1 = pw->x2 - pw->x1;
          pw->x1 = x - (x1 >> 1);
          pw->x2 = pw->x1 + x1;
        }
        else  pw->x1 = pw->x2 = x;
//        if(pw->Label) {	}
        insertWire(pw);
        break;

      case isDiagram:
        pe->cx = x - (pe->x2 >> 1);
        break;

      case isPainting:
        pe->getCenter(bx1, by1);
        pe->setCenter(x, by1, false);
        break;

      case isNodeLabel:
        pl = (WireLabel*)pe;
        if(((Element*)(pl->pOwner))->Type & isComponent)
          pe->cx += x - ((Component*)(pl->pOwner))->cx;
        else {
          pw = (Wire*)(pl->pOwner);
          if(pw->isHorizontal()) {
            x1 = pw->x2 - pw->x1;
            pe->cx += x - (x1 >> 1) - pw->x1;
          }
          else  pe->cx += x - pw->x1;
        }
        insertNodeLabel(pl);
        x += dx;
        break;

      default: ;
    }
    x -= dx;
  }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}

// ---------------------------------------------------
bool Schematic::distributeVertical()
{
  int x1, y1, x2, y2;
  int bx1, by1, bx2, by2;
  QList<Element *> ElementCache;
  int count = copyElements(x1, y1, x2, y2, ElementCache);
  if(count < 1) return false;

  Element *pe;
  // using bubble sort to get elements x ordered
  QMutableListIterator<Element *> ies(ElementCache);
  if(count > 1)
    for(int i = count-1; i>0; i--) {
      ies.toFront();
      pe = ies.next();
      for(int j=0; j<i; j++) {
        pe->getCenter(bx1, by1);
        pe = ies.next();
        pe->getCenter(bx2, by2);
        if(bx1 > bx2) {  // change two elements ?
          // current gets previous, step back one position
          ies.setValue(ies.previous());
          // new current gets temp 
          ies.setValue(pe);
          // step forward to initial position
          ies.next(); 
//          pe = ElementCache.next(); //Need another step?
        }
      }
    }
  ElementCache.last()->getCenter(x2, y2);
  ElementCache.first()->getCenter(x1, y1);
  Wire *pw;
  int y  = y2;
  int dy=0;
  if(count > 1) dy = (y2-y1)/(count-1);
  // re-insert elements and put them at right position
  // Go backwards in order to insert node labels before its component.
  QListIterator<Element *> ie2(ElementCache);
  ie2.toBack();
  while (ie2.hasPrevious()) {
    pe = ie2.previous();
    switch(pe->Type) {
      case isComponent:
      case isAnalogComponent:
      case isDigitalComponent:
        pe->cy = y;
        insertRawComponent((Component*)pe);
        break;

      case isWire:
        pw = (Wire*)pe;
        if(pw->isHorizontal())  pw->y1 = pw->y2 = y;
        else {
          y1 = pw->y2 - pw->y1;
          pw->y1 = y - (y1 >> 1);
          pw->y2 = pe->y1 + y1;
        }
//        if(pw->Label) {	}
        insertWire(pw);
        break;

      case isDiagram:
        pe->cy = y + (pe->y2 >> 1);
        break;

      case isPainting:
        pe->getCenter(bx1, by1);
        pe->setCenter(bx1, y, false);
        break;

      case isNodeLabel:
        if(((Element*)(((WireLabel*)pe)->pOwner))->Type & isComponent)
          pe->cy += y - ((Component*)(((WireLabel*)pe)->pOwner))->cy;
        else {
          pw = (Wire*)(((WireLabel*)pe)->pOwner);
          if(!pw->isHorizontal()) {
            y1 = pw->y2 - pw->y1;
            pe->cy += y - (y1 >> 1) - pw->y1;
          }
          else  pe->cy += y - pw->y1;
        }
        insertNodeLabel((WireLabel*)pe);
        y += dy;
        break;

      default: ;
    }
    y -= dy;
  }

  ElementCache.clear();
  if(count < 2) return false;

  setChanged(true, true);
  return true;
}


/* *******************************************************************
   *****                                                         *****
   *****                Actions with components                  *****
   *****                                                         *****
   ******************************************************************* */

// Finds the correct number for power sources, subcircuit ports and
// digital sources and sets them accordingly.
void Schematic::setComponentNumber(Component *c)
{
  Property *pp;
  
  if (c->Props.isEmpty()) return; // comp has no attributes
  else pp = c->Props.first();
  
  if(pp->Name != "Num") return; // has no Num attribute
  
  // current port number
  QString s = pp->Value;
  
  // First look, if the port number already exists.
  Component *pc;
  bool used = false;
  QListIterator<Component *> ic(Components);
  while (ic.hasNext()) {
    pc = ic.next();
    if(pc->Model == c->Model)
      if(pc->Props.first()->Value == pp->Value) {
          used = true;
          break;  // port number exists, find free number          
      }
  }
  if(!used) return;   // port number not in use 

  
  // Find the first free port number
  // Create list of existing port numbers
  QList<int> portList;
  QListIterator<Component *> ic2(Components);
  while (ic2.hasNext()) {
    pc = ic2.next();
    if(pc->Model == c->Model)
      portList.append(pc->Props.first()->Value.toInt());
  }
  
  // look on list for first available ports number
  int newN = 1;
  for(int n=1; n<=Components.size() + 1; n++) {
      if (!portList.contains(n)) {
          newN = n;
          break; //found it
      }
  }
   
  s  = QString::number(newN);
  pp->Value = s; // set new number
}

// ---------------------------------------------------
void Schematic::insertComponentNodes(Component *c, bool noOptimize)
{
  Port *pp = 0;
  // connect every node of the component to corresponding schematic node
//  for(pp = c->Ports.first(); pp != 0; pp = c->Ports.next())
  QListIterator<Port *> ip(c->Ports);
  while (ip.hasNext()) {
    pp = ip.next();
    pp->Connection = insertNode(pp->x+c->cx, pp->y+c->cy, c);
  }

  if(noOptimize)  return;

  Node    *pn = 0;
  Element *pe = 0;
  Element *pe1 = 0;
  QList<Element *> pL;
  // if component over wire then delete this wire
  
  
//  QListIterator<Port *>
  ip.toFront();
#warning why omit first???
//  c->Ports.first();  // omit the first element
  pp = ip.next();
  
  // loop thru remaining component ports
//  for(pp = c->Ports.next(); pp != 0; pp = c->Ports.next()) {
  while (ip.hasNext()) {
    pp = ip.next();
          

#warning solve this!
//    if (pp->Connection->Connections.size()) //resistor should return 1 here

    
    QListIterator<Element *> ie(pp->Connection->Connections);
    while (ie.hasNext()) {
      pe = ie.next();
      if(pe->Type == isWire) {
         if(((Wire*)pe)->Port1 == pn)  
           pL = (((Wire*)pe)->Port2->Connections);
         else  
           pL = (((Wire*)pe)->Port1->Connections);

//        for(pe1 = pL->first(); pe1!=0; pe1 = pL->next())
       for(int i=0; i < pL.size(); i++) {
           pe1 = pL[i];
          if(pe1 == c) {
            deleteWire((Wire*)pe);
            break;
          }
        }
      }
    }
  }
}

// ---------------------------------------------------
// Used for example in moving components.
void Schematic::insertRawComponent(Component *c, bool noOptimize)
{
  // connect every node of component to corresponding schematic node
  insertComponentNodes(c, noOptimize);
  Components.append(c);

  // a ground symbol erases an existing label on the wire line
  if(c->Model == "GND") {
    c->Model = "x";    // prevent that this ground is found as label
    Element *pe = getWireLabel(c->Ports.first()->Connection);
    if(pe) if((pe->Type & isComponent) == 0) {
      delete ((Conductor*)pe)->Label;
      ((Conductor*)pe)->Label = 0;
    }
    c->Model = "GND";    // rebuild component model
  }
}

// ---------------------------------------------------
void Schematic::recreateComponent(Component *Comp)
{
  Port *pp;
  WireLabel **plMem=0, **pl;
  int PortCount = Comp->Ports.size();

  if(PortCount > 0) {
    // Save the labels whose node is not connected to somewhere else.
    // Otherwise the label would be deleted.
    pl = plMem = (WireLabel**)malloc(PortCount * sizeof(WireLabel*));
    QListIterator<Port *> ip(Comp->Ports);
    while (ip.hasNext()) {
      pp = ip.next();
      if(pp->Connection->Connections.size() < 2) {
        *(pl++) = pp->Connection->Label;
        pp->Connection->Label = 0;
      }
      else  *(pl++) = 0;
    }
  }


  int x = Comp->tx, y = Comp->ty;
  int x1 = Comp->x1, x2 = Comp->x2, y1 = Comp->y1, y2 = Comp->y2;
  QString tmp = Comp->Name;    // is sometimes changed by "recreate"
  Comp->recreate(this);   // to apply changes to the schematic symbol
  Comp->Name = tmp;
  if(x < x1)
    x += Comp->x1 - x1;
  else if(x > x2)
    x += Comp->x2 - x2;
  if(y < y1)
    y += Comp->y1 - y1;
  else if(y > y2)
    y += Comp->y2 - y2;
  Comp->tx = x;  Comp->ty = y;


  if(PortCount > 0) {
    // restore node labels
    pl = plMem;
    QListIterator<Port *> ip(Comp->Ports);
    while (ip.hasNext()) {
      pp = ip.next();
      if(*pl != 0) {
        (*pl)->cx = pp->Connection->cx;
        (*pl)->cy = pp->Connection->cy;
        placeNodeLabel(*pl);
      }
      pl++;
      if((--PortCount) < 1)  break;
    }
    for( ; PortCount > 0; PortCount--) {
      delete (*pl);  // delete not needed labels
      pl++;
    }
    free(plMem);
  }
}

// ---------------------------------------------------
void Schematic::insertComponent(Component *c)
{
    qDebug() << "insertComponent";
    qDebug() << "DocComps  " << DocComps.size();
    qDebug() << "Components" << Components.size();
  // connect every node of component to corresponding schematic node
  insertComponentNodes(c, false);

  bool ok;
  QString s;
  int  max=1, len = c->Name.length(), z;
  if(c->Name.isEmpty()) {
    // a ground symbol erases an existing label on the wire line
    if(c->Model == "GND") {
      c->Model = "x";    // prevent that this ground is found as label
      Element *pe = getWireLabel(c->Ports.first()->Connection);
      if(pe) 
        if((pe->Type & isComponent) == 0) {
          delete ((Conductor*)pe)->Label;
          ((Conductor*)pe)->Label = 0;
        }
      c->Model = "GND";    // rebuild component model
    }
  }
  else {
    // determines the name by looking for names with the same
    // prefix and increment the number
    Component *pc;
    QListIterator<Component *> ic(Components);
    while (ic.hasNext()) {
      pc = ic.next();
      if(pc->Name.left(len) == c->Name) {
        s = pc->Name.right(pc->Name.length()-len);
        z = s.toInt(&ok);
        if(ok) if(z >= max) max = z + 1;
      }
    }
    c->Name += QString::number(max);  // create name with new number
  }

  setComponentNumber(c); // important for power sources and subcircuit ports
  Components.append(c);
}

// ---------------------------------------------------
void Schematic::activateCompsWithinRect(int x1, int y1, int x2, int y2)
{
  bool changed = false;
  int  cx1, cy1, cx2, cy2, a;
  // exchange rectangle coordinates to obtain x1 < x2 and y1 < y2
  cx1 = (x1 < x2) ? x1 : x2; cx2 = (x1 > x2) ? x1 : x2;
  cy1 = (y1 < y2) ? y1 : y2; cy2 = (y1 > y2) ? y1 : y2;
  x1 = cx1; x2 = cx2;
  y1 = cy1; y2 = cy2;

  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    pc->Bounding(cx1, cy1, cx2, cy2);
    if(cx1 >= x1) if(cx2 <= x2) if(cy1 >= y1) if(cy2 <= y2) {
      a = pc->isActive - 1;

      if(pc->Ports.size() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.first()->Connection);
      }
      changed = true;
    }
  }

  if(changed)  setChanged(true, true);
}

// ---------------------------------------------------
bool Schematic::activateSpecifiedComponent(int x, int y)
{
  int x1, y1, x2, y2, a;
  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    pc->Bounding(x1, y1, x2, y2);
    if(x >= x1) if(x <= x2) if(y >= y1) if(y <= y2) {
      a = pc->isActive - 1;

      if(pc->Ports.size() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.first()->Connection);
      }
      setChanged(true, true);
      return true;
    }
  }
  return false;
}

// ---------------------------------------------------
bool Schematic::activateSelectedComponents()
{
  int a;
  bool sel = false;
  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    if(pc->isSelected) {
      a = pc->isActive - 1;

      if(pc->Ports.size() > 1) {
        if(a < 0)  a = 2;
        pc->isActive = a;    // change "active status"
      }
      else {
        a &= 1;
        pc->isActive = a;    // change "active status"
        if(a == COMP_IS_ACTIVE)  // only for active (not shorten)
          if(pc->Model == "GND")  // if existing, delete label on wire line
            oneLabel(pc->Ports.first()->Connection);
      }
      sel = true;
    }
  }

  if(sel) setChanged(true, true);
  return sel;
}

// ---------------------------------------------------
// Sets the component ports anew. Used after rotate, mirror etc.
void Schematic::setCompPorts(Component *pc)
{
  Port *pp;
  WireLabel *pl;
  QList<WireLabel *> LabelCache;

  QListIterator<Port *> ip(pc->Ports);
  while (ip.hasNext()) {
    pp = ip.next();
    pp->Connection->Connections.removeOne((Element*)pc);// delete connections
    switch(pp->Connection->Connections.size()) {
      case 0:
        pl = pp->Connection->Label;
        if(pl) {
          LabelCache.append(pl);
          pl->cx = pp->x + pc->cx;
          pl->cy = pp->y + pc->cy;
        }
        Nodes.removeOne(pp->Connection);
        break;
      case 2:
        oneTwoWires(pp->Connection); // try to connect two wires to one
      default: ;
    }
  }

  // Re-connect component node to schematic node. This must be done completely
  // after the first loop in order to avoid problems with node labels.
//  for(pp = pc->Ports.first(); pp!=0; pp = pc->Ports.next())
  QListIterator<Port *> ip2(pc->Ports);
  while (ip2.hasNext()) {
    pp = ip2.next();
    pp->Connection = insertNode(pp->x+pc->cx, pp->y+pc->cy, pc);
  }

  for(int i=0; i<LabelCache.size(); i++ ) {
    pl = LabelCache.at(i);  
    insertNodeLabel(pl);
  }
}

// ---------------------------------------------------
// Returns a pointer of the component on whose text x/y points.
Component* Schematic::selectCompText(int x_, int y_, int& w, int& h)
{
  int a, b, dx, dy;
  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    a = pc->cx + pc->tx;
    if(x_ < a)  continue;
    b = pc->cy + pc->ty;
    if(y_ < b)  continue;

    pc->textSize(dx, dy);
    if(x_ > a+dx)  continue;
    if(y_ > b+dy)  continue;

    w = dx;
    h = dy;
    return pc;
  }

  return 0;
}

// ---------------------------------------------------
Component* Schematic::searchSelSubcircuit()
{
  Component *sub=0;
  // test all components
  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    if(!pc->isSelected) continue;
    if(pc->Model != "Sub")
      if(pc->Model != "VHDL")
	if(pc->Model != "Verilog") continue;

    if(sub != 0) return 0;    // more than one subcircuit selected
    sub = pc;
  }
  return sub;
}

// ---------------------------------------------------
Component* Schematic::selectedComponent(int x, int y)
{
  // test all components
  Component *pc;
  for(int i=0; i < Components.size(); i++) {
    pc = Components[i];
    if(pc->getSelected(x, y))
      return pc;
  }

  return 0;
}

// ---------------------------------------------------
// Deletes the component 'c'.
void Schematic::deleteComp(Component *c)
{
  Port *pn;

  // delete all port connections
  QListIterator<Port *> ip(c->Ports);
  while (ip.hasNext()) {
    pn = ip.next();
    switch(pn->Connection->Connections.size()) {
      case 1  : if(pn->Connection->Label) delete pn->Connection->Label;
                Nodes.removeOne(pn->Connection);  // delete open nodes
                pn->Connection = 0;		  //  (auto-delete)
                break;
      case 3  : pn->Connection->Connections.removeOne(c);// delete connection
                oneTwoWires(pn->Connection);  // two wires -> one wire
                break;
      default : pn->Connection->Connections.removeOne(c);// remove connection
                break;
    }
  }

  Components.removeOne(c);   // delete component
}

// ---------------------------------------------------
int Schematic::copyComponents(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  Component *pc;
  int bx1, by1, bx2, by2, count=0;
  // find bounds of all selected components
  QListIterator<Component *> ic(Components);
//  for(pc = Components->first(); pc != 0; ) {
  while (ic.hasNext()) {
    pc = ic.next();
    if(pc->isSelected) {
      pc->Bounding(bx1, by1, bx2, by2);  // is needed because of "distribute
      if(bx1 < x1) x1 = bx1;             // uniformly"
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;

      count++;
      ElementCache.append(pc);

      Port *pp;   // rescue non-selected node labels
      QListIterator<Port *> ip(pc->Ports);
      while (ip.hasNext()) {
        pp = ip.next();
        if(pp->Connection->Label)
          if(pp->Connection->Connections.size() < 2) {
            ElementCache.append(pp->Connection->Label);

            // Don't set pp->Connection->Label->pOwner=0,
            // so text position stays unchanged, but
            // remember component for align/distribute.
            pp->Connection->Label->pOwner = (Node*)pc;

            pp->Connection->Label = 0;
          }
      }

      deleteComp(pc);  //FIXME does not look right
//      pc = Components->current();
//      continue;
    }
//    pc = Components->next();
  }
  return count;
}

// ---------------------------------------------------
void Schematic::copyComponents2(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  Component *pc;
  // find bounds of all selected components
  QListIterator<Component *> ic(Components);
  while (ic.hasNext()) {
    pc = ic.next();
    if(pc->isSelected) {
      // is better for unsymmetrical components
      if(pc->cx < x1)  x1 = pc->cx;
      if(pc->cx > x2)  x2 = pc->cx;
      if(pc->cy < y1)  y1 = pc->cy;
      if(pc->cy > y2)  y2 = pc->cy;

      ElementCache.append(pc);

      Port *pp;   // rescue non-selected node labels
      QListIterator<Port *> ip(pc->Ports);
      while (ip.hasNext()) {
        pp = ip.next();
        if(pp->Connection->Label)
          if(pp->Connection->Connections.size() < 2) {
            ElementCache.append(pp->Connection->Label);
            pp->Connection->Label = 0;
            // Don't set pp->Connection->Label->pOwner=0,
            // so text position stays unchanged.
          }
       }

      deleteComp(pc); //FIXME doesn't look right
//      pc = Components->current();
//      continue;
    }
//    pc = Components->next();
  }
}


/* *******************************************************************
   *****                                                         *****
   *****                  Actions with labels                    *****
   *****                                                         *****
   ******************************************************************* */

// Test, if wire connects wire line with more than one label and delete
// all further labels. Also delete all labels if wire line is grounded.
void Schematic::oneLabel(Node *n1)
{
  qDebug() << "oneLabel";
  Wire *pw;
  Node *pn, *pNode;
  Element *pe;
  WireLabel *pl = 0;
  bool named=false;   // wire line already named ?
  QList<Node *> Cons;

//  for(pn = Nodes->first(); pn!=0; pn = Nodes->next())
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) {
    pn = in.next();
    pn->y1 = 0;   // mark all nodes as not checked
  }

  Cons.append(n1);
  n1->y1 = 1;  // mark Node as already checked
  for(int i=0; i<Cons.size(); i++ ) {
    pn = Cons.at(i);  
    if(pn->Label) {
      if(named) {
        delete pn->Label;
        pn->Label = 0;    // erase double names
      }
      else {
	    named = true;
	    pl = pn->Label;
      }
    }

    for(int i=0; i < pn->Connections.size(); i++) {
      pe = pn->Connections[i];
      if(pe->Type != isWire) {
        if(((Component*)pe)->isActive == COMP_IS_ACTIVE)
	  if(((Component*)pe)->Model == "GND") {
	    named = true;
	    if(pl) {
	      pl->pOwner->Label = 0;
	      delete pl;
	    }
	    pl = 0;
	  }
	continue;
      }
      pw = (Wire*)pe;

      if(pn != pw->Port1) pNode = pw->Port1;
      else pNode = pw->Port2;

      if(pNode->y1) continue;
      pNode->y1 = 1;  // mark Node as already checked
      Cons.append(pNode);
      //Cons.findRef(pn); //set back index to pn??

      if(pw->Label) {
        if(named) {
          delete pw->Label;
          pw->Label = 0;    // erase double names
        }
        else {
	  named = true;
	  pl = pw->Label;
	}
      }
    }
  }
}

// ---------------------------------------------------
int Schematic::placeNodeLabel(WireLabel *pl)
{
  Node *pn;
  int x = pl->cx;
  int y = pl->cy;

  // check if new node lies upon an existing node
  for(int i=0; i < Nodes.size(); i++)
    if(Nodes[i]->cx == x) if(Nodes[i]->cy == y) break;

  if(!pn)  return -1;

  Element *pe = getWireLabel(pn);
  if(pe) {    // name found ?
    if(pe->Type & isComponent) {
      delete pl;
      return -2;  // ground potential
    }

    delete ((Conductor*)pe)->Label;
    ((Conductor*)pe)->Label = 0;
  }

  pn->Label = pl;   // insert node label
  pl->Type = isNodeLabel;
  pl->pOwner = pn;
  return 0;
}

// ---------------------------------------------------
// Test, if wire line is already labeled and returns a pointer to the
// labeled element.
Element* Schematic::getWireLabel(Node *pn_)
{
  Wire *pw = 0;
  Node *pn = 0; 
  Node *pNode = 0;
  Element *pe = 0;
  QList<Node *> Cons;

  QListIterator<Node *> in(Nodes);
  while (in.hasNext()){
    pn = in.next();
    pn->y1 = 0;   // mark all nodes as not checked
  }

  Cons.append(pn_);
  pn_->y1 = 1;  // mark Node as already checked
  QListIterator<Node *> in2(Cons);
  while (in2.hasNext()) {
    pn = in2.next();
    if(pn->Label) 
      return pn;
    else {      
      QListIterator<Element *> ie(pn->Connections);
      while (ie.hasNext()) {
        pe = ie.next();
        qDebug() << "hasnext";
        if(pe->Type != isWire) {
	      if(((Component*)pe)->isActive == COMP_IS_ACTIVE)
              if(((Component*)pe)->Model == "GND") 
                  return pe;
          continue;
        }

        pw = (Wire*)pe;
        if(pw->Label) return pw;

        if(pn != pw->Port1) pNode = pw->Port1;
        else pNode = pw->Port2;

        if(pNode->y1) continue;
        pNode->y1 = 1;  // mark Node as already checked
        Cons.append(pNode);
//        Cons.findRef(pn);
      }
    }
  }
  return 0;   // no wire label found
}

// ---------------------------------------------------
// Inserts a node label.
void Schematic::insertNodeLabel(WireLabel *pl)
{
  if(placeNodeLabel(pl) != -1)
    return;

  // Go on, if label don't lie on existing node.
  Wire *pw = selectedWire(pl->cx, pl->cy);
  if(pw) {  // lies label on existing wire ?
    if(getWireLabel(pw->Port1) == 0)  // wire not yet labeled ?
      pw->setName(pl->Name, pl->initValue, 0, pl->cx, pl->cy);
    delete pl;
    return;
  }

  Node *pn = new Node(pl->cx, pl->cy);
  Nodes.append(pn);

  pn->Label = pl;
  pl->Type  = isNodeLabel;
  pl->pOwner = pn;
}

// ---------------------------------------------------
void Schematic::copyLabels(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  WireLabel *pl;
  // find bounds of all selected wires
  Wire *pw;
  QListIterator<Wire *> iw(Wires);
  while (iw.hasNext()) {
    pw = iw.next();
    pl = pw->Label;
    if(pl) if(pl->isSelected) {
      if(pl->x1 < x1) x1 = pl->x1;
      if(pl->y1-pl->y2 < y1) y1 = pl->y1-pl->y2;
      if(pl->x1+pl->x2 > x2) x2 = pl->x1+pl->x2;
      if(pl->y1 > y2) y2 = pl->y1;
      ElementCache.append(pl);
    }
  }
  
  Node *pn;
  QListIterator<Node *> in(Nodes);
  while (in.hasNext()) {
    pn = in.next();
    pl = pn->Label;
    if(pl) if(pl->isSelected) {
      if(pl->x1 < x1) x1 = pl->x1;
      if(pl->y1-pl->y2 < y1) y1 = pl->y1-pl->y2;
      if(pl->x1+pl->x2 > x2) x2 = pl->x1+pl->x2;
      if(pl->y1 > y2) y2 = pl->y1;
      ElementCache.append(pl);
      pl->pOwner->Label = 0;   // erase connection
      pl->pOwner = 0;
    }
  }
}


/* *******************************************************************
   *****                                                         *****
   *****                Actions with paintings                   *****
   *****                                                         *****
   ******************************************************************* */

Painting* Schematic::selectedPainting(float fX, float fY)
{
  float Corr = 5.0 / Scale; // size of line select

  Painting *pp;// = Paintings->first();
  QListIterator<Painting *> ip(Paintings);
  while(ip.hasNext()) {
    pp = ip.next();
    if(pp->getSelected(fX, fY, Corr))
      return pp;
  }

  return 0;
}

// ---------------------------------------------------
void Schematic::copyPaintings(int& x1, int& y1, int& x2, int& y2,
			QList<Element *> ElementCache)
{
  Painting *pp;
  int bx1, by1, bx2, by2;
  // find boundings of all selected paintings
#warning mutable?
  QListIterator<Painting *> ip(Paintings);
  while(ip.hasNext()) {
    pp = ip.next();
    if(pp->isSelected) {
      pp->Bounding(bx1, by1, bx2, by2);
      if(bx1 < x1) x1 = bx1;
      if(bx2 > x2) x2 = bx2;
      if(by1 < y1) y1 = by1;
      if(by2 > y2) y2 = by2;

      ElementCache.append(pp);
      Paintings.takeAt(Paintings.indexOf(pp));
//      pp = Paintings->current();
    }
//    else pp = Paintings->next();
  }
}
