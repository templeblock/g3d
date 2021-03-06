/**
  \file App.h

  The G3D 10.00 default starter app is configured for OpenGL 3.3 and
  relatively recent GPUs.
 */
#ifndef App_h
#define App_h
#include <G3D/G3D.h>

/** Application framework. */
class App : public GApp {
protected:

    shared_ptr<Model> createTorusModel();
    void addTorusToScene();

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit() override;
};

#endif
