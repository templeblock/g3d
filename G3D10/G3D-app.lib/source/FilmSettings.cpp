/**
  \file G3D-app.lib/source/FilmSettings.cpp

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#include "G3D-base/Any.h"
#include "G3D-app/GuiPane.h"
#include "G3D-gfx/GLCaps.h"
#include "G3D-app/FilmSettings.h"

namespace G3D {

FilmSettings::FilmSettings() : 
    m_gamma(2.2f),
    m_sensitivity(1.0f),
    m_bloomStrength(0.25f),
    m_bloomRadiusFraction(0.015f),
    m_antialiasingEnabled(true),
    m_antialiasingFilterRadius(0),
    m_antialiasingHighQuality(true),
    m_temporalAntialiasingEnabled(false),
    m_vignetteTopStrength(0.5f),
    m_vignetteBottomStrength(0.05f),
    m_vignetteSizeFraction(0.17f),
    m_debugZoom(1),
    m_effectsEnabled(true) {

    setIdentityToneCurve();
}


FilmSettings::FilmSettings(const Any& any) {
    *this = FilmSettings();
    AnyTableReader reader("FilmSettings", any);

    reader.getIfPresent("gamma", m_gamma);
    reader.getIfPresent("sensitivity", m_sensitivity);
    reader.getIfPresent("bloomStrength", m_bloomStrength);
    reader.getIfPresent("bloomRadiusFraction", m_bloomRadiusFraction);
    reader.getIfPresent("temporalAntialiasingEnabled", m_temporalAntialiasingEnabled);
    reader.getIfPresent("antialiasingEnabled", m_antialiasingEnabled);
    reader.getIfPresent("antialiasingFilterRadius", m_antialiasingFilterRadius);
    reader.getIfPresent("antialiasingHighQuality", m_antialiasingHighQuality);
    reader.getIfPresent("vignetteTopStrength", m_vignetteTopStrength);
    reader.getIfPresent("vignetteBottomStrength", m_vignetteBottomStrength);
    reader.getIfPresent("vignetteSizeFraction", m_vignetteSizeFraction);
    reader.getIfPresent("invertX", m_invertX);
    reader.getIfPresent("invertY", m_invertY);
    reader.getIfPresent("upscaleFilter", m_upscaleFilter);
    reader.getIfPresent("downscaleFilter", m_downscaleFilter);

    Any toneCurve;
    if (reader.getIfPresent("toneCurve", toneCurve)) {
        if (toneCurve.type() == Any::STRING) {
            if (toneCurve.string() == "IDENTITY") {
                setIdentityToneCurve();
            } else if (toneCurve.string() == "CONTRAST") {
                setContrastToneCurve();
            } else if (toneCurve.string() == "CELLULOID") {
                setCelluloidToneCurve();
            } else if (toneCurve.string() == "SUPERBOOST") {
                setSuperboostToneCurve();
            } else if (toneCurve.string() == "SATURATE") {
                setSaturateToneCurve();
            } else if (toneCurve.string() == "BURNOUT") {
                setBurnoutToneCurve();
            } else if (toneCurve.string() == "NEGATIVE") {
                setNegativeToneCurve();
            } else {
                toneCurve.verify(false, "Expected IDENTITY, CONTRAST, CELLULOID, SUPERBOOST, SATURATE, BURNOUT, NEGATIVE, or Spline<float>");
            }
        } else {
            m_toneCurve = Spline<float>(toneCurve);
        }
    }
    reader.getIfPresent("debugZoom", m_debugZoom);
    reader.getIfPresent("effectsEnabled", m_effectsEnabled);
    reader.verifyDone();
}


Any FilmSettings::toAny() const {
    Any any(Any::TABLE, "FilmSettings");

    any["gamma"] = m_gamma;
    any["sensitivity"] = m_sensitivity;
    any["bloomStrength"] = m_bloomStrength;
    any["bloomRadiusFraction"] = m_bloomRadiusFraction;
    any["temporalAntialiasingEnabled"] = m_temporalAntialiasingEnabled;
    any["antialiasingEnabled"] = m_antialiasingEnabled;
    any["antialiasingFilterRadius"] = m_antialiasingFilterRadius;
    any["antialiasingHighQuality"] = m_antialiasingHighQuality;
    any["vignetteTopStrength"] = m_vignetteTopStrength;
    any["vignetteBottomStrength"] = m_vignetteBottomStrength;
    any["vignetteSizeFraction"] = m_vignetteSizeFraction;
    any["toneCurve"] = m_toneCurve.toAny("Spline");
    any["debugZoom"] = m_debugZoom;
    any["effectsEnabled"] = m_effectsEnabled;
    any["invertX"] = m_invertX;
    any["invertY"] = m_invertY;
    any["upscaleFilter"] = m_upscaleFilter;
    any["downscaleFilter"] = m_downscaleFilter;

    return any;
}



void FilmSettings::makeGui(GuiPane* pane, float maxSensitivity, float sliderWidth, float indent) {
    pane->addCheckBox("Enabled", &m_effectsEnabled);

    GuiNumberBox<float>* n = nullptr;

    pane->addLabel("Exposure")->moveBy(indent, 0);
    n = pane->addNumberBox("Gamma",         &m_gamma, "", GuiTheme::LOG_SLIDER, 1.0f/2.2f, 7.0f, 0.01f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    n = pane->addNumberBox("Sensitivity",   &m_sensitivity, "", GuiTheme::LOG_SLIDER, 0.001f, maxSensitivity);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    GuiFunctionBox* fb = pane->addFunctionBox("Tone Curve", &m_toneCurve);
    fb->setSize(sliderWidth / 2, sliderWidth / 2);
    fb->moveBy(indent + 15, 0);
    fb->setTimeBounds(0.0f, 1.0f);
    fb->setValueBounds(0.0f, 1.0f); 
    fb->setCanChangeNumPoints(false);

    const float buttonWidth = sliderWidth / 2 - 35, buttonHeight = 18;
    GuiButton* b = pane->addButton("Identity", this, &FilmSettings::setIdentityToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 20);
    b->setSize(buttonWidth, buttonHeight);

    b = pane->addButton("Celluloid", this, &FilmSettings::setCelluloidToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 20 + buttonHeight);
    b->setSize(buttonWidth, buttonHeight);

    b = pane->addButton("Superboost", this, &FilmSettings::setSuperboostToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 20 + buttonHeight * 2);
    b->setSize(buttonWidth, buttonHeight);

    b = pane->addButton("Contrast", this, &FilmSettings::setContrastToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 20 + buttonHeight * 3);
    b->setSize(buttonWidth, buttonHeight);

    b = pane->addButton("Burnout", this, &FilmSettings::setBurnoutToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 20 + buttonHeight * 4);
    b->setSize(buttonWidth, buttonHeight);

    b = pane->addButton("Negative", this, &FilmSettings::setNegativeToneCurve, GuiTheme::TOOL_BUTTON_STYLE);
    b->setPosition(fb->rect().x1() + 30, fb->rect().y0() + 22 + buttonHeight * 5);
    b->setSize(buttonWidth, buttonHeight);
 
    pane->addLabel("Bloom")->moveBy(indent, 10);
    n = pane->addNumberBox("Strength",      &m_bloomStrength, "", GuiTheme::LOG_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    n = pane->addNumberBox("Radius",        &m_bloomRadiusFraction, "", GuiTheme::LOG_SLIDER, 0.0f, 0.2f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    pane->addLabel("Vignette")->moveBy(indent, 10);
    n = pane->addNumberBox("Top",           &m_vignetteTopStrength, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    n = pane->addNumberBox("Bottom",        &m_vignetteBottomStrength, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    n = pane->addNumberBox("Size",          &m_vignetteSizeFraction, "", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 15, 0);

    pane->addLabel("Antialiasing")->moveBy(indent, 10);
    GuiCheckBox* c = pane->addCheckBox("Screen-Space (FXAA)", &m_antialiasingEnabled);
    c->moveBy(indent + 13, 0);

    c = pane->addCheckBox("High Quality", &m_antialiasingHighQuality);
    c->moveBy(indent + 28, 0);

    n = pane->addNumberBox("Blur Radius", &m_antialiasingFilterRadius, "px", GuiTheme::LINEAR_SLIDER, 0.0f, 1.0f, 0.01f);
    n->setWidth(sliderWidth);  n->moveBy(indent + 28, 0);

    pane->addCheckBox("Temporal (TAA)", &m_temporalAntialiasingEnabled)->moveBy(indent + 13, 0);

    GuiNumberBox<int>* i = pane->addNumberBox("Debug Zoom",  &m_debugZoom, "x", GuiTheme::LINEAR_SLIDER, int(1), int(32));
    i->setWidth(sliderWidth);  i->moveBy(indent, 10);

    pane->beginRow();
    pane->addCheckBox("Invert X", &m_invertX)->moveBy(indent, 10);
    pane->addCheckBox("Invert Y", &m_invertY);
    pane->endRow();

    static const Array<String> filterList("NEAREST", "BILINEAR", "BICUBIC", "BICUBIC_SHARPER");
    GuiDropDownList* du = pane->addDropDownList("Upscale Filter", filterList, 
        Pointer<int>(
            [this]() { return int(m_upscaleFilter); },
            [this](int f) { m_upscaleFilter = ResampleFilter(f); }),
        nullptr);

    GuiDropDownList* dd = pane->addDropDownList("Downscale Filter", filterList,
        Pointer<int>(
            [this]() { return int(m_downscaleFilter); },
            [this](int f) { m_downscaleFilter = ResampleFilter(f); }),
        nullptr);

    du->setWidth(280); du->setCaptionWidth(120);
    dd->setWidth(280); dd->setCaptionWidth(120);

    pane->pack();
}


void FilmSettings::setIdentityToneCurve() {
    m_toneCurve.extrapolationMode = SplineExtrapolationMode::LINEAR;
    m_toneCurve.control.resize(5);
    m_toneCurve.time.resize(m_toneCurve.control.size());
    m_toneCurve.time[0] = 0.0f;
    m_toneCurve.time[1] = 0.1f;
    m_toneCurve.time[2] = 0.2f;
    m_toneCurve.time[3] = 0.5f;
    m_toneCurve.time[4] = 1.0f;
    for (int i = 0; i < m_toneCurve.control.size(); ++i) {
        m_toneCurve.control[i] = m_toneCurve.time[i];
    }
}


void FilmSettings::setContrastToneCurve() {
    m_toneCurve =
        Spline<float>(Any::parse(
            "Spline { "
            "    control = ( 0, 0.185106, 0.604255, 0.75532, 1 ); "
            "        extrapolationMode = LINEAR;" 
            "        finalInterval = -1; "
            "        interpolationMode = CUBIC; "
            "        time = ( 0, 0.3, 0.573914, 0.804348, 1 );" 
            "    };  "));
}


void FilmSettings::setCelluloidToneCurve() {
    m_toneCurve =
        Spline<float>(PARSE_ANY(
            Spline {
               control = ( 0, 0.0787234, 0.306383, 0.75532, 0.92);
               extrapolationMode = LINEAR;
               finalInterval = -1;
               interpolationMode = CUBIC;
               time = ( 0, 0.169565, 0.339131, 0.752174, 1 );
            }));
}


void FilmSettings::setSuperboostToneCurve() {
    m_toneCurve =
        Spline<float>(PARSE_ANY(
            Spline { 
                    control = ( 0, 0.0680851, 0.465957, 0.808511, 0.930638 ); 
                    extrapolationMode = "LINEAR"; 
                    finalInterval = -1; 
                    interpolationMode = "CUBIC"; 
                    time = ( 0, 0.126087, 0.330435, 0.795652, 1 ); 
            }));
}


void FilmSettings::setSaturateToneCurve() {
    m_toneCurve =
        Spline<float>(Any::parse(
         "Spline {"
         "  control = ( 0, 0.131915, 0.434043, 0.734043, 1 ); "
         "  extrapolationMode = LINEAR;" 
         "  finalInterval = -1; "
         "  interpolationMode = CUBIC; "
         "  time = ( 0, 0.0130435, 0.130435, 0.508696, 1 ); "
         "};"));
}


void FilmSettings::setBurnoutToneCurve() {
    m_toneCurve =
        Spline<float>(Any::parse(
            "Spline { "
            "  control = ( 0, 0.238298, 0.540425, 0.765958, 0.946809 ); "
            "  extrapolationMode = LINEAR; "
            "  finalInterval = -1; "
            "  interpolationMode = CUBIC; "
            "  time = ( 0, 0.456522, 0.579565, 0.691304, 1 ); "
            "};")); 
}


void FilmSettings::setNegativeToneCurve() {
    setIdentityToneCurve();
    for (int i = 0; i < m_toneCurve.control.size(); ++i) {
        m_toneCurve.control[i] = 1.0f - m_toneCurve.time[i];
    }
}


void FilmSettings::extendGBufferSpecification(GBuffer::Specification& spec) const {
    if (m_temporalAntialiasingEnabled && 
        (isNull(spec.encoding[GBuffer::Field::SS_POSITION_CHANGE].format) ||
               (spec.encoding[GBuffer::Field::SS_POSITION_CHANGE].format->redBits < 12))) {

        // Need higher precision than 8-bit
        spec.encoding[GBuffer::Field::SS_POSITION_CHANGE] =
            Texture::Encoding(GLCaps::supportsTexture(ImageFormat::RG16F()) ?
            ImageFormat::RG16F() :
            ImageFormat::RG32F(),
            FrameName::SCREEN, 1.0f, 0.0f);//, 16320.0f / 127.0f, -64.0f);
    }
}

} // namespace G3D
