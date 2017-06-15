#pragma once

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

    ofColor color = ofColor::white;

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
    ofShader    perlinShader;
    std::vector<ofImage> images;
    // ofTexture texture;
    ofTexture masktext;
    ofImage fgmask;
    cv::Mat cvmask;
    ofColor clearColor=ofColor::black;

    Pix_share pix_share;

    ofxOssia ossia;

    std::vector<Blob> blobs;

    ossia::ParameterGroup lineParam;
    ossia::Parameter<ofColor> lineColor; // couleur des lignes
    ossia::Parameter<ofVec2f> lineSize; // nombre de lignes verticales et horizontales à dessiner
    ossia::Parameter<float> lineWidth; // largeur des lignes à dessiner
    ossia::Parameter<float> lineRotation; // rotation
    float noiseamount=0; // noise amount
    float noisespeed=0.1; // changing speed
    float blobnoiseoffset=0.;
    double blobnoisetime=0.;
    double blobcoloroffset=0.;
    ofShader lineshader;
    bool dolineShader = true;
    ofVec2f scaleline = {1.,1.};
    std::vector<ofFloatImage> noises;

    float threshold, gain;
    std::vector<ofColor> blobColor = {ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.)};
    ofColor distanceColor = ofColor::white;
    ofColor noiseColor = ofColor::white;
    float blobnoiseamount = 1., blobnoisespeed=1.;
    int smokeAlpha[3];
    bool mask;
    bool showGui;
    float m_dist2luma, m_dist2noise;
    float noiseFreq=0.3, noiseSpeed = 1.0;

    void on2dPadEvent(ofxDatGui2dPadEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);
    void onToggleEvent(ofxDatGuiToggleEvent e);
    void onColorPickerEvent(ofxDatGuiColorPickerEvent e);


    void reload();
    void setupShader();
};
