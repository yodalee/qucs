/*
 * diode.cpp - diode class implementation
 *
 * Copyright (C) 2004 Stefan Jahn <stefan@lkcc.org>
 *
 * This is free software; you can redistribute it and/or modify
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
 * along with this package; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 *
 * $Id: diode.cpp,v 1.14 2004-06-09 06:27:08 ela Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "complex.h"
#include "object.h"
#include "node.h"
#include "circuit.h"
#include "net.h"
#include "analysis.h"
#include "dcsolver.h"
#include "component_id.h"
#include "constants.h"
#include "diode.h"

#define Cathode 1 /* cathode node */
#define Anode   2 /* anode node   */

diode::diode () : circuit (2) {
  rs = NULL;
  type = CIR_DIODE;
}

void diode::calcSP (nr_double_t frequency) {
  complex y = getOperatingPoint ("gd");
  y += rect (0, getOperatingPoint ("Cd") * 2.0 * M_PI * frequency);
  y *= z0; 
  setS (Cathode, Cathode, 1.0 / (1.0 + 2.0 * y));
  setS (Anode,     Anode, 1.0 / (1.0 + 2.0 * y));
  setS (Cathode,   Anode, 2.0 * y / (1.0 + 2.0 * y));
  setS (Anode,   Cathode, 2.0 * y / (1.0 + 2.0 * y));
}

void diode::initDC (dcsolver * solver) {

  // initialize starting values
  setV (Cathode, 0.0);
  setV (Anode, 0.9);
  Uprev = real (getV (Anode) - getV (Cathode));

  // possibly insert series resistance
  nr_double_t Rs = getPropertyDouble ("Rs");
  if (Rs != 0) {
    // create additional circuit if necessary and reassign nodes
    rs = splitResistance (this, rs, solver->getNet (), "Rs", "anode", Anode);
    applyResistance (rs, Rs);
  }
  // no series resistance
  else {
    disableResistance (this, rs, solver->getNet (), Anode);
  }
}

/* The function fills in the necessary values for all types of analyses
   into the given resistor circuit. */
void diode::applyResistance (circuit * res, nr_double_t Rs) {
  // apply constant MNA entries
  nr_double_t g = 1.0 / Rs;
  res->setY (1, 1, +g); res->setY (2, 2, +g);
  res->setY (1, 2, -g); res->setY (2, 1, -g);

  // apply constant S-Matrix
  nr_double_t r = Rs / z0;
  res->setS (1, 1, r / (r + 2.0)); res->setS (1, 2, 2.0 / (r + 2.0));
  res->setS (2, 2, r / (r + 2.0)); res->setS (2, 1, 2.0 / (r + 2.0));
}

/* This function can be used to create an extra resistor circuit.  If
   the 'res' argument is NULL then the new circuit is created, the
   nodes get re-arranged and it is inserted into the given
   netlist. The given arguments can be explained as follows.
   base:     calling circuit (this)
   res:      additional resistor circuit (can be NULL)
   subnet:   the netlist object
   c:        name of the additional circuit
   n:        name of the inserted (internal) node
   internal: number of new internal node (the orignal external node) */
circuit * diode::splitResistance (circuit * base, circuit * res, net * subnet,
				  char * c, char * n, int internal) {
  if (res == NULL) {
    res = new circuit (2);
    res->setName (createInternal (c, base->getName ()));
    res->setNode (1, base->getNode(internal)->getName ());
    res->setNode (2, createInternal (n, base->getName ()), 1);
    subnet->insertCircuit (res);
  }
  base->setNode (internal, res->getNode(2)->getName (), 1);
  return res;
}

/* This function is the counterpart of the above routine.  It removes
   the resistance circuit from the netlist and re-assigns the original
   node. */
void diode::disableResistance (circuit * base, circuit * res, net * subnet,
			       int internal) {
  if (res != NULL) {
    subnet->removeCircuit (res);
    base->setNode (internal, res->getNode(1)->getName (), 0);
  }
}

void diode::calcDC (void) {
  nr_double_t Is = getPropertyDouble ("Is");
  nr_double_t n  = getPropertyDouble ("N");
  nr_double_t Ud, Id, Ut, T, gd, Ieq, Ucrit, gtiny;

  T = -K + 26.5;
  Ut = T * kB / Q;
  Ud = real (getV (Anode) - getV (Cathode));

  // critical voltage necessary for bad start values
  Ucrit = n * Ut * log (n * Ut / M_SQRT2 / Is);
  Uprev = Ud = pnVoltage (Ud, Uprev, Ut * n, Ucrit);

  // tiny derivative for little junction voltage
  gtiny = Ud < - 10 * Ut * n ? Is : 0;

  gd = Is / Ut / n * exp (Ud / Ut / n) + gtiny;
  Id = Is * (exp (Ud / Ut / n) - 1) + gtiny * Ud;
  Ieq = Id - Ud * gd;

  setI (Cathode, +Ieq);
  setI (Anode,   -Ieq);

  setY (Cathode, Cathode, +gd); setY (Anode,   Anode, +gd);
  setY (Cathode,   Anode, -gd); setY (Anode, Cathode, -gd);
}

/* The function limits the forward pn-voltage for each DC iteration in
   order to avoid numerical overflows and thereby improve the
   convergence. */
nr_double_t diode::pnVoltage (nr_double_t Ud, nr_double_t Uold, nr_double_t Ut,
			      nr_double_t Ucrit) {
  nr_double_t arg;
  if (Ud > Ucrit && fabs (Ud - Uold) > 2 * Ut) {
    if (Uold > 0) {
      arg = 1 + (Ud - Uold) / Ut;
      Ud = arg > 0 ? Uold + Ut * log (arg) : Ucrit;
    }
    else Ud = Ut * log (Ud / Ut);
  }
  return Ud;
}

void diode::calcOperatingPoints (void) {
  nr_double_t Is  = getPropertyDouble ("Is");
  nr_double_t n   = getPropertyDouble ("N");
  nr_double_t z   = getPropertyDouble ("M");
  nr_double_t cj0 = getPropertyDouble ("Cj0");
  nr_double_t vd  = getPropertyDouble ("Vj");
  nr_double_t Tt  = getPropertyDouble ("Tt");
  
  nr_double_t Ud, Id, Ut, T, gd, Cd;

  T = -K + 26.5;
  Ut = kB * T / Q;
  Ud = real (getV (Anode) - getV (Cathode));
  gd = Is / Ut / n * exp (Ud / Ut / n);
  Id = Is * (exp (Ud / Ut / n) - 1);

  if (Ud < 0)
    Cd = cj0 * pow (1 - Ud / vd, -z);
  else
    Cd = cj0 * (1 + z * Ud / vd);
  Cd = Cd + Tt * gd;

  setOperatingPoint ("gd", gd);
  setOperatingPoint ("Id", Id);
  setOperatingPoint ("Vd", Ud);
  setOperatingPoint ("Cd", Cd);
}
