/**
  \file G3D-base.lib/source/ParseMTL.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-base/ParseMTL.h"
#include "G3D-base/TextInput.h"
#include "G3D-base/stringutils.h"
#include "G3D-base/FileSystem.h"
#include "G3D-base/Log.h"
#include "G3D-base/Any.h"

namespace G3D {

ParseMTL::Options::Options(const Any& a) {
    *this = Options();
    a.verifyName("ParseMTL::Options");
    AnyTableReader r(a);
    r.getIfPresent("defaultMapKs", defaultMapKs);
    r.getIfPresent("defaultKs", defaultKs);
    r.getIfPresent("defaultBumpMapIterations", defaultBumpMapIterations);    

    r.verifyDone();
}


Any ParseMTL::Options::toAny() const {
    Any a(Any::TABLE, "ParseMTL::Options");

    a["defaultMapKs"] = defaultMapKs;
    a["defaultKs"] = defaultKs;
    a["defaultBumpMapIterations"] = defaultBumpMapIterations;

    return a;
}

ParseMTL::ParseMTL() : m_isCurrentMaterialDissolveSet(false) {
    // Always provide a default material
    materialTable.set("default", Material::create());
}


void ParseMTL::parse(TextInput& ti, const String& basePath, const ParseMTL::Options& options) {
    m_options = options;

    materialTable.clear();

    m_basePath = basePath;
    if (m_basePath == "<AUTO>") {
        m_basePath = FilePath::parent(FileSystem::resolve(ti.filename()));
    }

    TextInput::Settings set;
    set.cppBlockComments = false;
    set.cppLineComments = false;
    set.otherCommentCharacter = '#';
    set.generateNewlineTokens = true;
    set.msvcFloatSpecials = false;
    set.sourceFileName = ti.filename();

    ti.pushSettings(set);

    // Always provide a default material
    materialTable.set("default", Material::create());

    while (ti.hasMore()) {
        // Consume comments/newlines
        while (ti.hasMore() && (ti.peek().type() == Token::NEWLINE)) {
            // Consume the newline
            ti.read();
        }

        if (ti.peek().type() == Token::END) {
            break;
        }

        // Process one line
        const String& cmd = ti.readSymbol();

        processCommand(ti, cmd);

        // Read until the end of the line if this line did not consume it
        while (ti.hasMore()) {
            const Token t = ti.read();
            if ((t.type() == Token::NEWLINE) || (t.type() == Token::END)) {
                break;
            }
        }
    }

    if (notNull(m_currentMaterial) && (m_currentMaterial->Ks.constant.r < 0.0f)) {
        m_currentMaterial->Ks.constant = m_options.defaultKs;
    }

    ti.popSettings();
}


static String removeLeadingSlash(const String& s) {
    if ((s.length() > 0) && isSlash(s[0])) {
        return s.substr(1);
    } else {
        return s;
    }
}


static void readColor3(TextInput& ti, Color3& c) {
    c.r = (float)ti.readNumber();
    c.g = (float)ti.readNumber();
    c.b = (float)ti.readNumber();
}


static void readMap(TextInput& ti, ParseMTL::Material::Field& field) {
    const Token t = ti.peek();
    if ((t.type() == Token::SYMBOL) && (t.string() == "-")) {
        // There are options coming
        ti.readSymbol("-");
        const String& opt = ti.readSymbol();
        if (opt == "mm") {
            // bias and gain
            field.mm.x = (float)ti.readNumber();
            field.mm.y = (float)ti.readNumber();
        } else if (opt == "bm") {
            field.mm.y = (float)ti.readNumber();
        }
    }
    field.map = removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString()));
}

void ParseMTL::processCommand(TextInput& ti, const String& cmd) {

    if (cmd == "newmtl") {

        if (notNull(m_currentMaterial) && (m_currentMaterial->Ks.constant.r < 0.0f)) {
            m_currentMaterial->Ks.constant = Color3(0.5f);
        }

        // Create a new material
        m_currentMaterial = Material::create();
        m_currentMaterial->name = trimWhitespace(ti.readUntilNewlineAsString());

        m_currentMaterial->basePath = m_basePath;
        materialTable.set(m_currentMaterial->name, m_currentMaterial);

        m_isCurrentMaterialDissolveSet = false;
    } else if (isNull(m_currentMaterial)) {
        logPrintf("Warning: encountered command with null material\n");
    } else if (cmd == "d") {
        // "dissolve"; alpha on range [0,1]
        if (ti.peek().type() == Token::SYMBOL) {
            // Optional "-halo" 
            ti.readSymbol();
        }
        
        const float d = (float)ti.readNumber();
        // Only warn if incompatible overloaded settings are used
        if (m_isCurrentMaterialDissolveSet && (m_currentMaterial->d != d)) {
            debugPrintf("Warning: file \"%s\" uses the \"d\" command to set the dissolve parameter of material \"%s\", but it was already set by a preceding \"d\" or \"Tr\" command.\n", ti.filename().c_str(), m_currentMaterial->name.c_str());
        }

        m_currentMaterial->d = d;
        m_isCurrentMaterialDissolveSet = true;

    } else if (cmd == "Tr") {
        // Nonstandard 1 - alpha on range [0,1]

        const float d = 1.0f - (float)ti.readNumber();
        // Only warn if incompatible overloaded settings are used
        if (m_isCurrentMaterialDissolveSet && (m_currentMaterial->d != d)) {
            debugPrintf("Warning: file \"%s\" uses the \"Tr\" command to set the dissolve parameter of material \"%s\", but it was already set by a preceding \"d\" or \"Tr\" command.\n", ti.filename().c_str(), m_currentMaterial->name.c_str());
        }
        
        m_isCurrentMaterialDissolveSet = true;
        m_currentMaterial->d = d;
        
    } else if (cmd == "Ns") {
        // Specular Exponent
        m_currentMaterial->Ns = (float)ti.readNumber();
    } else if (cmd == "Ni") {
        // Index of Refraction, should be >= 1
        m_currentMaterial->Ni = (float)ti.readNumber();
    } else if (cmd == "Ka") {
        readColor3(ti, m_currentMaterial->Ka.constant);
    } else if ((cmd == "Kd") || (cmd == "kd")) {
        readColor3(ti, m_currentMaterial->Kd.constant);
    } else if (cmd == "Ks") {
        readColor3(ti, m_currentMaterial->Ks.constant);
    } else if (cmd == "Ke") {
        readColor3(ti, m_currentMaterial->Ke.constant);
    } else if (cmd == "Tf") {
        readColor3(ti, m_currentMaterial->Tf);
    } else if (cmd == "illum") {
        m_currentMaterial->illum = ti.readInteger();
    } else if (cmd == "map_Ke") {
        readMap(ti, m_currentMaterial->Ke);
    } else if (cmd == "map_Ka") {
        readMap(ti, m_currentMaterial->Ka);
    } else if ((cmd == "map_Kd") || (cmd == "map_kd")) {
        readMap(ti, m_currentMaterial->Kd);
    } else if ((cmd == "map_d") || (cmd == "map_D")) {
        m_currentMaterial->map_d = removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString()));
    } else if (cmd == "lightMap") { 
        // Non-standard G3D extension
        m_currentMaterial->lightMap = removeLeadingSlash(trimWhitespace(ti.readUntilNewlineAsString()));
    } else if (cmd == "map_Ks") {
        readMap(ti, m_currentMaterial->Ks);

        // We default Ks to -1 because we want to default it to 1 if there 
        // is a map_Ks and 0.5f otherwise (which then gets raised to the ninth power by G3D)
        // We thus have to check and properly set the default whenever we finish parsing a material or
        // assign map_Ks
        if (m_currentMaterial->Ks.constant.r < 0) {
            m_currentMaterial->Ks.constant = m_options.defaultMapKs;
        }
    } else if ((cmd == "map_bump") || (cmd == "bump") || (cmd == "map_Bump")) {
        readMap(ti, m_currentMaterial->bump);
    } else if (cmd == "interpolateMode") {
        m_currentMaterial->interpolateMode = ti.readSymbol();
        ti.readUntilNewlineAsString();
    } else {
        ti.readUntilNewlineAsString();
        debugPrintf("Ignoring unrecognized command in MTL file %s at line %d: '%s'\n", ti.filename().c_str(), ti.peekLineNumber(), cmd.c_str());
    }
}

} // namespace G3D
