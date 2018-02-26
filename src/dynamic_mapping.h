#pragma once


#include <opencv2/stitching.hpp>

#include "ofMain.h"
#include "ofxLibdc.h"
#include "ofxQuadWarp.h"
#include "ofxDatGui.h"
#include "Pix_share.h"
#include "ofxCv.h"
#include "ofxOssia.h"

struct Blob {
    int id;
    ofVec2f centroid;
    float area;
    ofRectangle bounding_box;
    ofVec2f velocity=ofVec2f(12,25);
    ofVec2f m_scale=ofVec2f(320,240);
    float distance=34.;
    float age=0;

    ofFloatColor color = ofColor::white;

    void draw();
};

class dynamic_mapping : public ofBaseApp
{
  public:
    void setup  ();
    void update ();
    void draw   ();
    void exit();

    void keyPressed (ofKeyEventArgs&);

private:
    ofFbo linefbo;
    ofRectangle sourceRect;
    ofxQuadWarp warper;
    ofFbo fbo;
    ofxDatGui gui;
    ofShader    shader;
    // ofTexture texture;
    ofTexture masktext;
    ofImage fgmask;
    cv::Mat cvmask;
    ofColor clearColor=ofColor::black;

    Pix_share pix_share;

    ofxOssia ossia;
    // ossia client
    ossia::net::generic_device* client_device{};
    ossia::net::local_protocol client_local_proto;

    std::vector<Blob> blobs;

    ossia::ParameterGroup lineParam, drawParam, inputParam;
    ossia::Parameter<ofColor> lineColor; // couleur des lignes
    ossia::Parameter<float> lineGap; // espace entre les lignes
    ossia::Parameter<float> lineWidth; // largeur des lignes à dessiner
    ossia::Parameter<float> lineRotation; // rotation
    ossia::Parameter<ofVec2f> lineOffset; // line grid XY offset
    ossia::Parameter<ofVec2f> lineNoiseSpeed; // changing speed
    ossia::Parameter<float> lineNoiseAmount; // noise amount
    ossia::Parameter<int> lineResolution;

    ossia::Parameter<float> inputGain;
    ossia::Parameter<int> inputThreshold;
    ossia::Parameter<bool> drawLines, drawBlobs, drawInputImage, drawMask;
    ossia::Parameter<ofColor> blobColor[10];

    // ossia::Parameter<ofQuaternion> test;

    uint64 lastTime=0;
    float blobnoiseoffset=0.;
    double blobnoisetime=0.;
    double blobcoloroffset=0.;
    ofShader lineshader;
    bool dolineShader = true;
    ofVec2f scaleline = {1.,1.};
    std::vector<ofFloatImage> noises;
    unsigned int max_length;

    ofColor distanceColor = ofColor::white;
    ofColor noiseColor = ofColor::white;
    float blobnoiseamount = 1., blobnoisespeed=1.;
    int smokeAlpha[3];
    bool showGui;
    float m_dist2luma, m_dist2noise;
    float noiseFreq=0.3, noiseSpeed = 1.0;

    void on2dPadEvent(ofxDatGui2dPadEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);
    void onToggleEvent(ofxDatGuiToggleEvent e);
    void onColorPickerEvent(ofxDatGuiColorPickerEvent e);

    void reload();
    void setupShader();
    void connect_to_voxelstrack();

    void setupGui();
};
