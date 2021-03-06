/**
  \file G3D-app.lib/source/GuiRadioButton.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-app/GuiRadioButton.h"
#include "G3D-app/GuiWindow.h"

namespace G3D {

GuiRadioButton::GuiRadioButton(GuiContainer* parent, const GuiText& text, int myID, 
                               const Pointer<int>& value, GuiTheme::RadioButtonStyle style) 
    : GuiControl(parent, text), m_value(value), m_myID(myID), m_style(style) {
}


void GuiRadioButton::setSelected() {
    if (*m_value != m_myID) {
        *m_value = m_myID;
        fireEvent(GEventType::GUI_ACTION);
    }
}


void GuiRadioButton::render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const {
    (void)rd;
    
    if (m_visible) {
        if (m_style == GuiTheme::NORMAL_RADIO_BUTTON_STYLE) {
            theme->renderRadioButton(m_rect, m_enabled && ancestorsEnabled, focused() || mouseOver(), selected(), m_caption);
        } else {
            theme->renderButton(m_rect, m_enabled && ancestorsEnabled, focused() || mouseOver(), selected(), m_caption,
                               (m_style == GuiTheme::BUTTON_RADIO_BUTTON_STYLE) ? 
                               GuiTheme::NORMAL_BUTTON_STYLE : GuiTheme::TOOL_BUTTON_STYLE);
        }
    }
}


void GuiRadioButton::setRect(const Rect2D& rect) {
    if (m_style == GuiTheme::NORMAL_RADIO_BUTTON_STYLE) {
        // Prevent the radio button from stealing clicks very far away
        m_rect = rect;
        m_clickRect = theme()->radioButtonToClickBounds(m_rect, m_caption);
    } else {
        GuiControl::setRect(rect);
    }
}


bool GuiRadioButton::selected() const {
    return *m_value == m_myID;
}


bool GuiRadioButton::onEvent(const GEvent& event) {
    if (m_visible) {
        if (event.type == GEventType::MOUSE_BUTTON_DOWN) {
            setSelected();
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

}
