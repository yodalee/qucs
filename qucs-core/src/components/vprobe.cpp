/*
 * vprobe.cpp - AC/DC and transient voltage probe class implementation
 *
 * Copyright (C) 2006 Stefan Jahn <stefan@lkcc.org>
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
 * the Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.  
 *
 * $Id: vprobe.cpp,v 1.1 2006/02/06 09:50:15 raimi Exp $
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "complex.h"
#include "object.h"
#include "node.h"
#include "circuit.h"
#include "component_id.h"
#include "vprobe.h"

vprobe::vprobe () : circuit (2) {
  type = CIR_VPROBE;
  setProbe (true);
}

void vprobe::initSP (void) {
  allocMatrixS ();
}

void vprobe::initDC (void) {
  allocMatrixMNA ();
}

void vprobe::saveOperatingPoints (void) {
  nr_double_t V = real (getV (NODE_1) - getV (NODE_2));
  setOperatingPoint ("V", V);
}

void vprobe::initAC (void) {
  initDC ();
}

void vprobe::initTR (void) {
  initDC ();
}