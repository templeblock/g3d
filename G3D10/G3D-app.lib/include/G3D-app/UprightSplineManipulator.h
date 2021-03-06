/**
  \file G3D-app.lib/include/G3D-app/UprightSplineManipulator.h

  G3D Innovation Engine http://casual-effects.com/g3d
  Copyright 2000-2019, Morgan McGuire
  All rights reserved
  Available under the BSD License
*/
#ifndef G3D_UprightSplineManipulator_h
#define G3D_UprightSplineManipulator_h

#include "G3D-base/platform.h"
#include "G3D-base/Color3.h"
#include "G3D-base/UprightFrame.h"
#include "G3D-base/ReferenceCount.h"
#include "G3D-base/g3dmath.h"
#include "G3D-app/Widget.h"

namespace G3D {


/**
 Allows recording and playback of a G3D::UprightSpline based on a G3D::Camera.
 Used by G3D::CameraControlWindow.

 \sa G3D::UprightFrame, G3D::UprightSpline, G3D::FirstPersonManipulator
 */
class UprightSplineManipulator : public Manipulator {
public:
    
    /**
       RECORD_INTERVAL_MODE: Automatically add one frame per
       second to the spline, reading from the camera.

       RECORD_KEY_MODE: Add one frame every time the recordKey() is
       pressed.

       PLAY_MODE: Move the frame() through the spline over time.  Does not mutate the
       camera.

       INACTIVE_MODE: Do nothing.
       
       The manipulator begins in INACTIVE_MODE.
     */
    enum Mode {RECORD_INTERVAL_MODE, RECORD_KEY_MODE, PLAY_MODE, INACTIVE_MODE};

protected:

    UprightSpline          m_spline;

    /** Current time during playback. */
    RealTime               m_time;
    
    Mode                   m_mode;

    shared_ptr<class Camera> m_camera;

    CoordinateFrame        m_currentFrame;

    bool                   m_showPath;

    Color3                 m_pathColor;

    /** samples per second */
    const float            m_sampleRate;

    GKey                   m_recordKey;

    UprightSplineManipulator();

    // Intentionally unimplemented
    UprightSplineManipulator& operator=(const UprightSplineManipulator&);
public:
    
    static shared_ptr<UprightSplineManipulator> create(const shared_ptr<class Camera>& c = shared_ptr<Camera>());
    
    virtual void onNetwork() {}
    virtual void onAI() {}

    /** Returns the underlying spline. */
    const UprightSpline& spline() const {
        return m_spline;
    }

    void setSpline(const UprightSpline& s) {
        m_spline = s;
    }
    
    GKey recordKey() const {
        return m_recordKey;
    }

    void setRecordKey(GKey k) {
        m_recordKey = k;
    }
    
    void setExtrapolationMode(SplineExtrapolationMode::Value m) {
        m_spline.extrapolationMode = m;
    }

    SplineExtrapolationMode::Value extrapolationMode() const {
        return m_spline.extrapolationMode.value;
    }

    /** If set to true, the camera's path is shown as a glowing curve. */
    void setShowPath(bool b) {
        m_showPath  = b;
    }

    bool showPath() const {
        return m_showPath;
    }
    
    void setPathColor(const Color3& c) {
        m_pathColor = c;
    }

    const Color3& pathColor() const {
        return m_pathColor;
    }

    /** Setting the mode to RECORD_KEY_MODE and RECORD_INTERVAL_MODE
        require that a non-nullptr camera have previously been set. When
        in INACTIVE_MODE, time does not elapse for the manipulator.*/
    void setMode(Mode m);

    Mode mode() const;

    /** Number of control points */
    int splineSize() const {
        return m_spline.control.size();
    }

    const shared_ptr<Camera>& camera() const {
        return m_camera;
    }
    
    /** Destroys the current path */
    void clear();

    /** Time since the beginning of recording or playback. */
    void setTime(double t);

    double time() const {
        return m_time;
    }

    /**
       Sets the camera from which the frame will be read when recording.  
       Argument may be nullptr, but recording cannot proceed until this is non-null.

       This camera will also be used for playback
     */
    void setCamera(const shared_ptr<Camera>& c) {
        m_camera = c;
    }

    shared_ptr<Camera> camera() {
        return m_camera;
    }

    virtual CoordinateFrame frame() const;
    virtual void getFrame(CoordinateFrame &c) const;
    virtual void onPose(Array< shared_ptr<Surface> > &posedArray, Array< shared_ptr<Surface2D> >& posed2DArray);
    virtual bool onEvent (const GEvent &event);
    virtual void onSimulation (RealTime rdt, SimTime sdt, SimTime idt);
    virtual void onUserInput (UserInput *ui);

    
};

}

#endif
