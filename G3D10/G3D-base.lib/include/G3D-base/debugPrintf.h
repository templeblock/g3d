/**
  \file G3D-base.lib/include/G3D-base/debugPrintf.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_DEBUGPRINTF_H
#define G3D_DEBUGPRINTF_H

#include "G3D-base/platform.h"
#include <stdio.h>
#include <cstdarg>
#include "G3D-base/format.h"
#include "G3D-base/G3DString.h"

namespace G3D {

typedef void (*ConsolePrintHook)(const String&);

namespace _internal {
    extern ConsolePrintHook _consolePrintHook;
}

/** Called by consolePrintf after the log and terminal have been written to.
 Used by GConsole to intercept printing routines.*/
void setConsolePrintHook(ConsolePrintHook h);

ConsolePrintHook consolePrintHook();

/**
   Sends output to the log and to the last GConsole instantiated.

   Guarantees that the output has been flushed by the time the routine
   returns.
   @sa G3D::logPrintf, G3D::screenPrintf
   @return The string that was printed
 */
String consolePrintf(const char* fmt ...) G3D_CHECK_PRINTF_ARGS;
String consolePrint(const String&);

/**
   Under Visual Studio, appears in the Debug pane of the Output window
   On Unix-based operating systems the output is sent to stderr.

   Also sends output to the console (G3D::consolePrintf) if there is a consolePrintHook, and flushes before returning.   

   \return The string that was printed
*/
String debugPrintf(const char* fmt ...) G3D_CHECK_PRINTF_ARGS;

} // namespace G3D

#endif

