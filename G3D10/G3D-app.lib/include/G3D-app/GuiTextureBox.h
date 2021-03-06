/**
  \file G3D-app.lib/include/G3D-app/GuiTextureBox.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#pragma once
#define G3D_GuiTextureBox_h

#include "G3D-base/platform.h"
#include "G3D-base/Vector2.h"
#include "G3D-gfx/Texture.h"
#include "G3D-app/GuiContainer.h"
#include "G3D-app/GuiText.h"

namespace G3D {

class GuiPane;
class GApp;
class GuiButton;
class GuiTextureBoxInspector;
class HeartbeatWidget;

class GuiTextureBox : public GuiContainer {
public:
    friend class GuiTextureBoxInspector;

protected:

    /** Padding around the image */
    enum {BORDER = 1};

    shared_ptr<Texture>             m_texture;

    shared_ptr<CallbackWidget>      m_recordWidget;

    weak_ptr<GuiTextureBoxInspector>m_inspector;

    Texture::Visualization          m_settings;

    /** Bounds for mouse clicks and scissor region, updated by every render. */
    Rect2D                          m_clipBounds;
    
    bool                            m_showInfo;

    bool                            m_showCubemapEdges;

    /** If true, textures are drawn with the Y coordinate inverted.
        Ignored if drawing a cube map. */
    bool                            m_drawInverted;

    /** Cached formatting of m_lastSize */
    mutable GuiText                 m_lastSizeCaption;
    mutable Vector2int16            m_lastSize;
    mutable const ImageFormat*      m_lastFormat;

    float                           m_zoom;
    Vector2                         m_offset;

    /** True when dragging the image */
    bool                            m_dragging;
    Vector2                         m_dragStart;
    Vector2                         m_offsetAtDragStart;

    /** When recording a movie */
    shared_ptr<class VideoOutput>   m_videoOutput;

    /** Readback texel */
    mutable Color4                  m_texel;

    /** Readback position */
    mutable Vector2int16            m_readbackXY;

    /** If true, this is the texture box inside of the inspector and should not be a button */
    bool                            m_embeddedMode;

    /** Returns the bounds of the canvas (display) region for this GuiTextBox */
    Rect2D canvasRect() const;

    /** Returns the bounds of the canvas (display) region for a GuiTextBox of size \a rect*/
    Rect2D canvasRect(const Rect2D& rect) const;

    void drawTexture(RenderDevice* rd, const Rect2D& r) const;

    void computeSizeString() const;
    
    /** Compute a new Texture that is the result of applying all of the visualization post-processing.
        This is used for producing images to save. The normal rendering path does not call this function. */
    shared_ptr<Texture> applyProcessing() const;

public:

    /** In most cases, you'll want to call GuiPane::addTextureBox instead.

      \param embeddedMode When set to true, hides the controls that duplicate inspector functionality.
     */
    GuiTextureBox
    (GuiContainer*       parent,
     const GuiText&      caption,
     GApp*               app,
     const shared_ptr<Texture>& t = shared_ptr<Texture>(),
     bool                embeddedMode = false,
     bool                drawInverted = false);

    virtual ~GuiTextureBox();

    GApp*                           m_app;

    /** Starts the inspector window.  Invoked by the inspector button. */
    void showInspector();

    /** Zoom factor for the texture display.  Greater than 1 = zoomed in. */
    inline float viewZoom() const {
        return m_zoom;
    }
    
    void setViewZoom(float z);

    /** Offset of the texture from the centered position. Positive = right and down. */
    inline const Vector2& viewOffset() const {
        return m_offset;
    }

    void zoomIn();
    void zoomOut();
    
    /** Brings up the modal save dialog to save an image with post-processing */
    void save();

    /** Brings up the modal save dialog to save the underlying data */
    void rawSave();

    void setViewOffset(const Vector2& x);

    /** Change the scale to 1:1 pixel */
    void zoomTo1();

    /** Center the image and scale it to fill the viewport */
    void zoomToFit();

    /** If the texture was previously nullptr, also invokes zoomToFit() */
    void setTexture(const shared_ptr<Texture>& t);
    void setTexture(const shared_ptr<Texture>& t, bool drawInverted);
    void setSettings(const Texture::Visualization& s);

    /** Set m_drawInverted */
    void setInverted(const bool inverted) {
        m_drawInverted = inverted;
    }

    inline const shared_ptr<Texture>& texture() const {
        return m_texture;
    }

    inline const Texture::Visualization& settings() const {
        return m_settings;
    }

    /** Controls the display of (x,y)=rgba when the mouse is over the box.
        Defaults to true.  Note that displaying these values can significantly
        impact performance because it must read back from the GPU to the CPU.*/
    inline void setShowInfo(bool b) {
        m_showInfo = b;
    }

    inline bool showInfo() const {
        return m_showInfo;
    }

    /** Sizes the control so that exactly \a dims of viewing space is available. 
        Useful for ensuring that textures are viewed at 1:1.*/
    void setSizeFromInterior(const Vector2& dims);

    virtual void render(RenderDevice* rd, const shared_ptr<GuiTheme>& theme, bool ancestorsEnabled) const override;
    virtual void setRect(const Rect2D& rect) override;
    virtual void findControlUnderMouse(Vector2 mouse, GuiControl*& control) override;

    virtual bool onEvent(const GEvent& event) override;

    /** Invoked by the drawer button. Do not call directly. */
    void toggleDrawer();

    /* Bind arguments to the specified shader */
    void setShaderArgs(UniformTable& args);

    virtual void setCaption(const GuiText& text) override;

    /** True if movie recording */
    bool movieRecording() const {
        return notNull(m_videoOutput);
    }

    /** Start/stop movie recording. Stopping brings up the save box. Destroying the GuiTextureBox cancels automatically. */
    void setMovieRecording(bool b);

};

} // namespace
