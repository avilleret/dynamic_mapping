#pragma once

#include "ofMain.h"
#include "ofxLibdc.h"
#include "ofxQuadWarp.h"
#include "ofxDatGui.h"

class ofApp : public ofBaseApp
{
  public:
    void setup  ();
    void update ();
    void draw   ();
    void exit();

    void keyPressed (ofKeyEventArgs&);

private:
    ofxLibdc::Camera camera;
    ofImage curFrame;
    ofxQuadWarp warper;
    ofFbo fbo;
    ofxDatGui gui;

    float threshold, gain;

    void on2dPadEvent(ofxDatGui2dPadEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);

    void reload();
};
