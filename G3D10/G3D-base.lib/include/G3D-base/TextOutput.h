/**
  \file G3D-base.lib/include/G3D-base/TextOutput.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#ifndef G3D_TextOutput_h
#define G3D_TextOutput_h

#include "G3D-base/platform.h"
#include "G3D-base/Array.h"
#include "G3D-base/G3DString.h"

namespace G3D {

/**
  Convenient formatting of ASCII text written to a file.
  <P>

  The core writeString, writeNumber, and writeSymbol methods map to TextInput's
  methods.  Number and Symbol each print an additional space that is used to 
  separate adjacent tokens.
  
  TextOutput::printf allows arbitrary text to be conveniently dumped
  en-masse.  Use [de]serialize(bool, TextOutput) and other overloads to read/write
  primitive types in a standardized manner and 

  <P>
  When a word-wrap line break occurs, all whitespace between words is replaced 
  with a single newline (the newline may be two characters-- see 
  G3D::TextOutput::Options::NewlineStyle).  Word wrapping occurs against
  the number of columns specified by Options::numColumns, <I>minus</I> the current
  indent level.

  Indenting adds the specified number of spaces immediately after a newline.
  If a newline was followed by spaces in the original string, these are added
  to the indent spaces.  Indenting <B>will</B> indent blank lines and will leave
  indents after the last newline of a file (if the indent level is non-zero at the end).

  <P><B>Serialization/Marshalling</B>
  <DT>Text serialization is accomplished using TextOutput by defining the pair of 
  methods:

  <PRE>
  void serialize(TextOutput& to) const;
  void deserialize(TextInput& ti);
  </PRE>

  See also G3D::TextInput.

  <P>
  <B>BETA API</B>
  <DT>This API is subject to change in future versions.
 */
class TextOutput {
public:

    class Settings {
    public:
        /** 
          WRAP_NONE             Word wrapping is disabled
          WRAP_WITHOUT_BREAKING Word-wrap, but don't break continuous lines that
                                are longer than numColumns (default)
          WRAP_ALWAYS           Wrap even if it means breaking a continuous line or
                                a quoted string.

          Word wrapping is only allowed at whitespaces ('\\n', '\\r', '\\t', ' '); it
          will not occur after commas, punctuation, minus signs, or any other characters
        */
        enum WordWrapMode {WRAP_NONE, WRAP_WITHOUT_BREAKING, WRAP_ALWAYS};

        /** Defaults to WRAP_WITHOUT_BREAKING */
        WordWrapMode        wordWrap;

        /** Is word-wrapping allowed to insert newlines inside double quotes?
            Default: false */
        bool                allowWordWrapInsideDoubleQuotes;

        /** Number of columns for word wrapping. Default: 8 */
        int                 numColumns;

        /** Number of spaces in each indent. Default: 4 */
        int                 spacesPerIndent;

        /** Style of newline used by word wrapping and by (optional) conversion.
            default: Windows: NEWLINE_WINDOWS, Linux, OS X: NEWLINE_UNIX.
          */
        enum NewlineStyle {NEWLINE_WINDOWS, NEWLINE_UNIX};

        NewlineStyle        newlineStyle;

        /** If true, all newlines are converted to NewlineStyle regardless of 
            how they start out. Default: true. */
        bool                convertNewlines;

        /** Used by writeBoolean */
        String         trueSymbol;

        /** Used by writeBoolean */
        String         falseSymbol;

        Settings() :
            wordWrap(WRAP_WITHOUT_BREAKING),
            allowWordWrapInsideDoubleQuotes(false),
            numColumns(80),
            spacesPerIndent(4),
            convertNewlines(true),
            trueSymbol("true"),
            falseSymbol("false") {
            #ifdef G3D_WINDOWS
                newlineStyle = NEWLINE_WINDOWS;
            #else
                newlineStyle = NEWLINE_UNIX;
            #endif
        }
    };

private:

    /** Used by indentAndAppend to tell when we are writing the
        first character of a new line. 
      
        So that push/popIndent work correctly, we cannot indent
        immediately after writing a newline.  Instead we must
        indent on writing the first character <B>after</B> that 
        newline.
      */
    bool                    startingNewLine;

    /** Number of characters at the end of the buffer since the last newline */
    int                     currentColumn;

    /** True if we have seen an open " and no close ".*/
    bool                    inDQuote;

    /** Empty if there is none */
    String             filename;

    Array<char>             data;

    Settings                option;

    /** Number of indents to prepend before each line.  Always set using setIndentLevel.*/
    int                     indentLevel;

    void setIndentLevel(int i);

    /** Actual number of spaces to indent. */
    int                     indentSpaces;

    /** the newline character(s) */
    String             newline;

    /** Starts at 1 */
    int                     m_currentLine;

    void setOptions(const Settings& _opt);

    /** Converts to the desired newlines.  Called from vprintf */
    void convertNewlines(const String& in, String& out);

    /** Called from vprintf */
    void wordWrapIndentAppend(const String& str);

    /** Appends the character to data, indenting whenever a newline is encountered.
        Called from wordWrapIndentAppend */
    void indentAppend(char c);

public:

    explicit TextOutput(const String& filename, const Settings& options = Settings());

    /** Constructs a text output that can later be commited to a string instead of a file.*/
    explicit TextOutput(const Settings& options = Settings());

    /** Returns one plus the number of newlines written since the output was created. */
    int line() const {
        return m_currentLine;
    }

    int column() const {
        return currentColumn;
    }

    /** Commit to the filename specified on the constructor. 
         <B>Not</B> called from the destructor; you must call
     it yourself.
    @param flush If true (default) the file is ready for reading when the method returns, otherwise 
     the method returns immediately and writes the file in the background.*/
    void commit(bool flush = true);

    /** Commits to this string */
    void commitString(String& string);

    /** Increase indent level by 1 */
    void pushIndent();

    void popIndent();

    /** Produces a new string that contains the output */
    String commitString();

    /** Writes a quoted string. Special characters in the string (e.g., \\, \\t, \\n) are escaped so that 
        TextInput will produce the identical string on reading.*/
    void writeString(const String& string);

    void writeBoolean(bool b);

    void writeNumber(double n);

    void writeNumber(int n);

    /** Guaranteed to parse as a C syntax double */
    void writeCNumber(double n, bool trailingSpace = true, bool minimal = false);

    /** Guaranteed to parse as a C syntax float */
    void writeCNumber(float n, bool trailingSpace = true, bool minimal = false);

    /** Guaranteed to parse as a C syntax int */
    void writeCNumber(int n, bool trailingSpace = true, bool minimal = false);

    void writeNewline();

    void writeNewlines(int numLines);

    /** If the most recently written character was a space, remove it and return true. Can be called repeatedly to back up over multiple spaces. */
    bool deleteSpace();

    /** The symbol is written without quotes.  Symbols are required to begin with a
        letter or underscore and contain only letters, underscores, and numbers 
        or be a C++ symbol (e.g. "{", "(", "++", etc.)
        so that they may be properly parsed by TextInput::readSymbol. Symbols are
        printed with a trailing space.*/
    void writeSymbol(const String& string);

    void writeSymbol(char s);

    /** Convenient idiom for writing multiple symbols in a row, e.g.
        writeSymbols("name", "=");  The empty symbols are not written.
        */
    void writeSymbols(
        const String& a,
        const String& b = "",
        const String& c = "",
        const String& d = "",
        const String& e = "",
        const String& f = "");

    /** Normal printf conventions.  Note that the output will be reformatted
        for word-wrapping and newlines */
    void printf(const char* fmt, ...)
        G3D_CHECK_PRINTF_METHOD_ARGS;

    // Can't pass by reference because that confuses va_start
    void printf(const String fmt, ...);
    void vprintf(const char* fmt, va_list argPtr) 
        G3D_CHECK_VPRINTF_METHOD_ARGS;
};

// Primitive serializers
void serialize(const bool& b, TextOutput& to);
void serialize(const int& b, TextOutput& to);
void serialize(const uint8& b, TextOutput& to);
void serialize(const double& b, TextOutput& to);
void serialize(const float& b, TextOutput& to);
void serialize(const String& b, TextOutput& to);
void serialize(const char* b, TextOutput& to);

}

#endif
