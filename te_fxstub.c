/* TECO for Ultrix    Copyright 1987 Matt Fichtenbaum                   */
/* This program and its components belong to GenRad, Concord, MA  01742 */
/* They may be copied if this copyright notice is included              */

/* te_fxstub.c    dummy stub for FX    10/12/87         */

/* stub for teco's "external" command FX                */
/* this just causes an "invalid F command" error        */
/* other routines can implement other commands for FX   */

#include "te_defs.h"

te_fx()
{
    ERROR(E_IFC);
}
