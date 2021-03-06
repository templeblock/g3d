/**
  \file G3D-app.lib/include/G3D-app/GuiScrollPane.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/

#pragma once

#include "G3D-app/GuiScrollBar.h"
#include "G3D-base/platform.h"
#include "G3D-base/Array.h"
#include "G3D-base/Pointer.h"
#include "G3D-app/GuiContainer.h"
 
namespace G3D {

class GuiPane;

/**
A conventional pane with scroll bars that allow the
user to determine which part of the pane to view.
**/
class GuiScrollPane : public GuiContainer {
protected:
    
    GuiPane*                    m_viewPane;
    GuiScrollBar*               m_verticalScrollBar;
    GuiScrollBar*               m_horizontalScrollBar;
    GuiTheme::ScrollPaneStyle   m_style;

    bool                        horizontalEnabled;
    bool                        verticalEnabled;
    float                       m_verticalOffset;
    float                       m_horizontalOffset;

    virtual bool onEvent(const GEvent& event) override { (void)event; return false; }

    float borderStartBump() const {
        return 2 * theme()->textBoxBorderWidth();
    }

    float borderDimensionsBump() const {
        return 2 * theme()->textBoxBorderWidth();
    }

    float scrollBarStartBump() const {
        return theme()->textBoxBorderWidth();
    }

    float scrollBarDimesionsBump() const {
        return 2*(theme()->textBoxBorderWidth());
    }

public:


    GuiScrollPane(GuiContainer* parent, bool verticalScroll, bool horizontalScroll, GuiTheme::ScrollPaneStyle style);

    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override;

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    
    virtual void setRect(const Rect2D& rect) override;
    
    virtual void pack();
    
    GuiPane* viewPane() const {
        return m_viewPane;
    }

    float verticalOffset() const {
        return m_verticalOffset;
    }

    float horizontalOffset() const {
        return m_horizontalOffset;
    }

    Vector2 paneOffset() const {
        return Vector2(m_horizontalOffset, m_verticalOffset);
    }

};

}
