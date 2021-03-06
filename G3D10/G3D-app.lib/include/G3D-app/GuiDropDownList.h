/**
  \file G3D-app.lib/include/G3D-app/GuiDropDownList.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#pragma once

#include "G3D-base/Pointer.h"
#include "G3D-base/Array.h"
#include "G3D-app/GuiWindow.h"
#include "G3D-app/GuiControl.h"
#include "G3D-app/GuiMenu.h"

namespace G3D {

class GuiPane;

/** List box for viewing strings or GuiText.  

    Fires a G3D::GuiEvent of type G3D::GEventType::GUI_ACTION on 
    the containing window when the user selects a new value, 
    GEventType::GUI_CANCEL when the user opens the dropdown and
    then clicks off or presses ESC.
*/
class GuiDropDownList : public GuiControl {
    friend class GuiWindow;
    friend class GuiPane;

protected:

    /** Pop-up list menu; call menu() to create this. */
    shared_ptr<GuiMenu>             m_menu;
    
    shared_ptr<GuiMenu>             menu();

    /** The index of the currently selected item. */
    Pointer<int>                    m_indexValue;

    /** m_indexValue points to this if no external pointer was provided. */
    int                             m_myInt;

    /** Reserved for future use: If m_listValuePtr is nullptr, use m_listValue (which is more fully featured anyway)
    */
    Pointer< Array<String> >        m_listValuePtr;
    Array<GuiText>                  m_listValue;

    /** True when the menu is open */
    bool                            m_selecting;

    GuiControl::Callback            m_actionCallback;

    bool                            m_usePrefixTreeMenus;

    virtual bool onEvent(const GEvent& event) override;
   
    /** Makes the menu appear */
    void showMenu();

public:

    /**For use in creating compound controllers. For everthing else use GuiPane::addDropDownList **/
    GuiDropDownList
       (GuiContainer*               parent, 
        const GuiText&              caption, 
        const Pointer<int>&         indexValue,
        const Array<GuiText>&       listValue,
        const Pointer< Array<String> >&  listValuePtr,
        const GuiControl::Callback& actionCallback,
        bool                        usePrefixTreeMenus);
       
    /** Called by GuiPane **/
    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    
    void setList(const Array<GuiText>& c);

    void setList(const Array<String>& c);

    /** Remove all values from the list */
    void clear();

    void append(const GuiText& c);

    const GuiText& get(int i) const {
        return m_listValue[i];
    }

    void set(int i, const GuiText& v) {
        m_listValue[i] = v;
        m_menu.reset();
    }

    int numElements() const {
        return m_listValue.size();
    }

    virtual void setRect(const Rect2D&) override;

    /** Returns the currently selected value */
    const GuiText& selectedValue() const;

    /** True if the list contains this value */
    bool containsValue(const String& s) const;

    /** The index of the currently selected value; -1 if the list is empty */
    int selectedIndex() const {
        if (m_listValue.size() == 0) {
            return -1;
        } else {
            return iClamp(*m_indexValue, 0, m_listValue.size() - 1);
        }
    }
    
    void setSelectedIndex(int i) {
        *m_indexValue = i;
    }

    /** Selects the first value whose text() is equal to \a s. If not
        found, leaves the index unchanged. */
    void setSelectedValue(const String& s);

    void resize(int n) {
        m_listValue.resize(n);
        *m_indexValue = selectedIndex();
        m_menu.reset();
    }

};

} // G3D
