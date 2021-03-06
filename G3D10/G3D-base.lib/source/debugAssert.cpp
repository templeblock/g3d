/**
  \file G3D-base.lib/source/debugAssert.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#include "G3D-base/debugAssert.h"
#include "G3D-base/platform.h"
#ifdef G3D_WINDOWS
    #include <tchar.h>
#endif
#include "G3D-base/format.h"
#include "G3D-base/prompt.h"
#include "G3D-base/G3DString.h"
#include "G3D-base/debugPrintf.h"
#include "G3D-base/Log.h"

#include <cstdlib>

#ifdef _MSC_VER
    // disable: "C++ exception handler used"
#   pragma warning (push)
#   pragma warning (disable : 4530)
#endif

using namespace std;

namespace G3D { namespace _internal {

ConsolePrintHook _consolePrintHook;
AssertionHook _debugHook = _handleDebugAssert_;
AssertionHook _failureHook = _handleErrorCheck_;

#ifdef G3D_LINUX
    Display*      x11Display = nullptr;
    Window        x11Window  = 0;
#endif


#ifdef G3D_WINDOWS
static void postToClipboard(const char *text) {
    if (OpenClipboard(nullptr)) {
        HGLOBAL hMem = GlobalAlloc(GHND | GMEM_DDESHARE, strlen(text) + 1);
        if (hMem) {
            char *pMem = (char*)GlobalLock(hMem);
            strcpy(pMem, text);
            GlobalUnlock(hMem);

            EmptyClipboard();
            SetClipboardData(CF_TEXT, hMem);
        }

        CloseClipboard();
        GlobalFree(hMem);
    }
}
#endif

/**
 outTitle should be set before the call
 */
static void createErrorMessage(
    const char*         expression,
    const String&  message,
    const char*         filename,
    int                 lineNumber,
    String&        outTitle,
    String&        outMessage) {

    String le = "";
    const char* newline = "\n";

    #ifdef G3D_WINDOWS
        newline = "\r\n";

        // The last error value.  (Which is preserved across the call).
        DWORD lastErr = GetLastError();
    
        // The decoded message from FormatMessage
        LPTSTR formatMsg = nullptr;

        if (nullptr == formatMsg) {
            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_IGNORE_INSERTS |
                          FORMAT_MESSAGE_FROM_SYSTEM,
                            nullptr,
                            lastErr,
                            0,
                            (LPTSTR)&formatMsg,
                            0,
                            nullptr);
        }

        // Make sure the message got translated into something.
        LPTSTR realLastErr;
        if (nullptr != formatMsg) {
            realLastErr = formatMsg;
        } else {
            realLastErr = _T("Last error code does not exist.");
        }

        if (lastErr != 0) {
            le = G3D::format("Last Error (0x%08X): %s\r\n\r\n", lastErr, (LPCSTR)realLastErr);
        }

        // Get rid of the allocated memory from FormatMessage.
        if (nullptr != formatMsg) {
            LocalFree((LPVOID)formatMsg);
        }

        char modulePath[MAX_PATH];
        GetModuleFileNameA(nullptr, modulePath, MAX_PATH);

        const char* moduleName = strrchr(modulePath, '\\');
        outTitle = outTitle + String(" - ") + String(moduleName ? (moduleName + 1) : modulePath);

    #endif

    // Build the message.
    outMessage =
        G3D::format("%s%s%sExpression: %s%s%s:%d%s%s%s", 
                 message.c_str(), newline, newline, expression, newline, 
                 filename, lineNumber, newline, newline, le.c_str());
}


bool _handleDebugAssert_(
    const char*         expression,
    const String&  message,
    const char*         filename,
    int                 lineNumber,
    bool                useGuiPrompt) {

    String dialogTitle = "Assertion Failure";
    String dialogText = "";
    createErrorMessage(expression, message, filename, lineNumber, dialogTitle, dialogText);

    #ifdef G3D_WINDOWS
        DWORD lastErr = GetLastError();
        postToClipboard(dialogText.c_str());
        debugPrintf("\n%s\n", dialogText.c_str());
    #endif

    const int cBreak        = 0;
    const int cIgnore       = 1;
    const int cAbort        = 2;

    static const char* choices[] = {"Debug", "Ignore", "Exit"};

    // Log the error
    Log::common()->print(String("\n**************************\n\n") + dialogTitle + "\n" + dialogText);

    const int result = G3D::prompt(dialogTitle.c_str(), dialogText.c_str(), (const char**)choices, 3, useGuiPrompt);

#    ifdef G3D_WINDOWS
        // Put the incoming last error back.
        SetLastError(lastErr);
#    endif

    switch (result) {
    // -1 shouldn't actually occur because it means 
    // that we're in release mode.
    case -1:
    case cBreak:
        return true;
        break;

    case cIgnore:
        return false;
        break;
   
    case cAbort:
        exit(-1);
        break;
    }

    // Should never get here
    return false;
}


bool _handleErrorCheck_(
    const char*         expression,
    const String&  message,
    const char*         filename,
    int                 lineNumber,
    bool                useGuiPrompt) {

    String dialogTitle = "Critical Error";
    String dialogText = "";

    createErrorMessage(expression, message, filename, lineNumber, dialogTitle, dialogText);

    // Log the error
    Log::common()->print(String("\n**************************\n\n") + dialogTitle + "\n" + dialogText);
    #ifdef G3D_WINDOWS
        DWORD lastErr = GetLastError();
        (void)lastErr;
        postToClipboard(dialogText.c_str());
        debugPrintf("\n%s\n", dialogText.c_str());
    #endif

    static const char* choices[] = {"Ok"};

    const String& m = 
        String("An internal error has occured in this program and it will now close.  "
        "The specific error is below. More information has been saved in \"") +
            Log::getCommonLogFilename() + "\".\n\n" + dialogText;

    int result = G3D::prompt("Error", m.c_str(), (const char**)choices, 1, useGuiPrompt);
    (void)result;

    return true;
}


#ifdef G3D_WINDOWS
static HCURSOR oldCursor;
static RECT    oldCursorRect;
static POINT   oldCursorPos;
static int     oldShowCursorCount;
#endif

void _releaseInputGrab_() {
    #ifdef G3D_WINDOWS

        GetCursorPos(&oldCursorPos);

        // Stop hiding the cursor if the application hid it.
        oldShowCursorCount = ShowCursor(true) - 1;

        if (oldShowCursorCount < -1) {
            for (int c = oldShowCursorCount; c < -1; ++c) {
                ShowCursor(true);
            }
        }

        // Set the default cursor in case the application
        // set the cursor to nullptr.
        oldCursor = GetCursor();
        SetCursor(LoadCursor(nullptr, IDC_ARROW));

        // Allow the cursor full access to the screen
        GetClipCursor(&oldCursorRect);
        ClipCursor(nullptr);
        
    #elif defined(G3D_LINUX)
        if (x11Display != nullptr) {
            XUngrabPointer(x11Display, CurrentTime);
            XUngrabKeyboard(x11Display, CurrentTime);
            if (x11Window != 0) {
                //XUndefineCursor(x11Display, x11Window);
                // TODO: Note that we leak this cursor; it should be
                // freed in the restore code.
                Cursor c = XCreateFontCursor(x11Display, 68);
                XDefineCursor(x11Display, x11Window, c);
            }
            XSync(x11Display, false);           
            XAllowEvents(x11Display, AsyncPointer, CurrentTime);
            XFlush(x11Display);
        }
    #elif defined(G3D_OSX)
        // TODO: OS X
    #endif
}


void _restoreInputGrab_() {
    #ifdef G3D_WINDOWS

        // Restore the old clipping region
        ClipCursor(&oldCursorRect);

        SetCursorPos(oldCursorPos.x, oldCursorPos.y);

        // Restore the old cursor
        SetCursor(oldCursor);

        // Restore old visibility count
        if (oldShowCursorCount < 0) {
            for (int c = 0; c > oldShowCursorCount; --c) {
                ShowCursor(false);
            }
        }
        
    #elif defined(G3D_LINUX)
        // TODO: Linux
    #elif defined(G3D_OSX)
        // TODO: OS X
    #endif
}


}; // internal namespace
 
void setAssertionHook(AssertionHook hook) {
    G3D::_internal::_debugHook = hook;
}

AssertionHook assertionHook() {
    return     G3D::_internal::_debugHook;
}

void setFailureHook(AssertionHook hook) {
    G3D::_internal::_failureHook = hook;
}

AssertionHook failureHook() {
    return G3D::_internal::_failureHook;
}


void setConsolePrintHook(ConsolePrintHook h) {
    G3D::_internal::_consolePrintHook = h;
}

ConsolePrintHook consolePrintHook() {
    return G3D::_internal::_consolePrintHook;
}


static String debugPrint(const String& s) {
#   ifdef G3D_WINDOWS
        const int MAX_STRING_LEN = 1024;
    
        // Windows can't handle really long strings sent to
        // the console, so we break the string.
        if (s.size() < MAX_STRING_LEN) {
            OutputDebugStringA(s.c_str());
        } else {
            for (unsigned int i = 0; i < s.size(); i += MAX_STRING_LEN) {
                const String& sub = s.substr(i, MAX_STRING_LEN);
                OutputDebugStringA(sub.c_str());
            }
        }
#    else
        fprintf(stderr, "%s", s.c_str());
        fflush(stderr);
#    endif

     return s;
}

String debugPrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    String s = G3D::vformat(fmt, argList);
    va_end(argList);

    return debugPrint(s);
//    return debugPrint(consolePrint(s));
}

String consolePrint(const String& s) {
    FILE* L = Log::common()->getFile();
    fprintf(L, "%s", s.c_str());

    if (consolePrintHook()) {
        consolePrintHook()(s);
    }

    fflush(L);
    return s;
}


String consolePrintf(const char* fmt ...) {
    va_list argList;
    va_start(argList, fmt);
    String s = G3D::vformat(fmt, argList);
    va_end(argList);

    return consolePrint(s);
}

} // namespace

#ifdef _MSC_VER
#   pragma warning (pop)
#endif
