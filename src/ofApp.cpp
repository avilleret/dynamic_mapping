#include "ofApp.h"

void ofApp::setup()
{
    camera.setup();

    int w = camera.getWidth();
    int h = camera.getHeight();
    int x = (ofGetWidth() - w) * 0.5;       // center on screen.
    int y = (ofGetHeight() - h) * 0.5;     // center on screen.

    fbo.allocate(w,h);

    warper.setSourceRect(ofRectangle(0, 0, w, h));              // this is the source rectangle which is the size of the image and located at ( 0, 0 )
    warper.setTargetRect(ofRectangle(x,y,w,h));
    warper.setup();

    gui.add2dPad("top left", ofRectangle(0, 0, w, h));
    gui.add2dPad("top right", ofRectangle(0, 0, w, h));
    gui.add2dPad("bottom right", ofRectangle(0, 0, w, h));
    gui.add2dPad("bottom left", ofRectangle(0, 0, w, h));
    gui.addBreak();
    gui.addSlider("threshold",0,255,127);
    gui.addSlider("gain",0.,10.,1.);

    gui.addHeader("Dynamic Mapping");
    gui.addFooter();

    gui.on2dPadEvent(this, &ofApp::on2dPadEvent);
    gui.onSliderEvent(this, &ofApp::onSliderEvent);

    reload();
}

void ofApp::update()
{
    if(camera.grabVideo(curFrame)) {
        curFrame.update();
    }
}

void ofApp::draw()
{
    // If the camera isn't ready, the curFrame will be empty.
    if(!camera.isReady()) return;

    // Camera doesn't draw itself, curFrame does.

    fbo.begin();
    curFrame.draw(0, 0);
    fbo.end();

    //======================== get our quad warp matrix.

    ofMatrix4x4 mat = warper.getMatrix();

    //======================== use the matrix to transform our fbo.

    ofPushMatrix();
    ofMultMatrix(mat);
    fbo.draw(0, 0);
    ofPopMatrix();

    if (gui.getAutoDraw()){
        warper.draw();
    }
}

void ofApp::exit(){
}

void ofApp::on2dPadEvent(ofxDatGui2dPadEvent e){
    vector<ofPoint> line(warper.getSourcePoints());

    if (e.target->is("top left")) {
        line[0].x = e.x;
        line[0].y = e.y;
    } else if (e.target->is("top right")) {
        line[1].x = e.x;
        line[1].y = e.y;
    } else if (e.target->is("bottom right")) {
        line[2].x = e.x;
        line[2].y = e.y;
    } else if (e.target->is("bottom left")) {
        line[3].x = e.x;
        line[3].y = e.y;
    }

    warper.setSourcePoints(line);
}

void ofApp::onSliderEvent(ofxDatGuiSliderEvent e){

    if (e.target->is("threshold")) threshold = e.value;
    else if (e.target->is("gain")) gain = e.value;
}

void ofApp::keyPressed(ofKeyEventArgs& key)
{
    // TODO move that ugly part into a class that inherit from ofxDatGui2dPad
    ofxDatGui2dPad * pad = 0;
    if ( (pad = gui.get2dPad("top left")) && pad->getFocused()) ;
    else if ((pad = gui.get2dPad("top right")) && pad->getFocused()) ;
    else if ((pad = gui.get2dPad("bottom right")) && pad->getFocused()) ;
    else if ((pad = gui.get2dPad("bottom left")) && pad->getFocused()) ;

    if ( pad ){
        switch ( key.key ){
        case OF_KEY_LEFT:
            pad->setPoint(ofPoint(pad->getPoint().x-1, pad->getPoint().y));
            break;
        case OF_KEY_UP:
            pad->setPoint(ofPoint(pad->getPoint().x, pad->getPoint().y-1));
            break;
        case OF_KEY_RIGHT:
            pad->setPoint(ofPoint(pad->getPoint().x+1, pad->getPoint().y));
            break;
        case OF_KEY_DOWN:
            pad->setPoint(ofPoint(pad->getPoint().x, pad->getPoint().y+1));
            break;
        default:
            ;
        }
        pad->dispatchEvent();
    }

    switch(key.key){
    case 'r': // reset warper;
    {
        int w = camera.getWidth();
        int h = camera.getHeight();
        int x = (ofGetWidth() - w) * 0.5;       // center on screen.
        int y = (ofGetHeight() - h) * 0.5;     // center on screen.
        ofxDatGui2dPad * pad = gui.get2dPad("top left");
        pad->setPoint(ofPoint(1,1));
        pad->dispatchEvent();
        pad = gui.get2dPad("top right");
        pad->setPoint(ofPoint(w-1,1));
        pad->dispatchEvent();
        pad = gui.get2dPad("bottom right");
        pad->setPoint(ofPoint(w-1,h-1));
        pad->dispatchEvent();
        pad = gui.get2dPad("bottom left");
        pad->setPoint(ofPoint(1,h-1));
        pad->dispatchEvent();

        warper.setTargetRect(ofRectangle(x,y,w,h));
        break;
    }
    case 'h':
        gui.setAutoDraw(!gui.getAutoDraw());
        break;
    case 's':
        warper.save();
        break;
    case 'l':
        reload();
    default:
        ;
    }
}

void ofApp::reload(){
    warper.load(); // reload last saved changes.)
    vector<ofPoint> src = warper.getSourcePoints();

    ofxDatGui2dPad * pad = gui.get2dPad("top left");
    pad->setPoint(src[0]);
    pad = gui.get2dPad("top right");
    pad->setPoint(src[1]);
    pad = gui.get2dPad("bottom right");
    pad->setPoint(src[2]);
    pad = gui.get2dPad("bottom left");
    pad->setPoint(src[3]);
}
