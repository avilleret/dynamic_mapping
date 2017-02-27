#pragma once

#include "ofMain.h"
#include "ofxLibdc.h"
#include "ofxQuadWarp.h"
#include "ofxDatGui.h"
#include "Pix_share.h"
#include "ofxOsc.h"
#include "ofxCv.h"

struct Blob {
    int id;
    ofVec2f centroid;
    float area;
    ofRectangle bounding_box;
    ofVec2f velocity;
    ofVec2f m_scale=ofVec2f(320,240);

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

    ofxOscReceiver receiver, pd;

    std::vector<Blob> blobs;


    ofColor lineColor=ofColor::white; // couleur des lignes
    int hline=12,vline=0; // nombre de lignes verticales et horizontales à dessiner
    float wline=5; // largeur des lignes à dessiner

    float threshold, gain;
    std::vector<ofColor> blobColor = {ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.), ofColor(0.,0.,0.,0.)};
    int smokeAlpha[3];
    bool mask;
    bool showGui;

    void on2dPadEvent(ofxDatGui2dPadEvent e);
    void onSliderEvent(ofxDatGuiSliderEvent e);
    void onToggleEvent(ofxDatGuiToggleEvent e);
    void onColorPickerEvent(ofxDatGuiColorPickerEvent e);


    void reload();
    void setupShader();
};
