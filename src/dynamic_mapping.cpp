#include "ofApp.h"

void ofApp::setup()
{
    camera.setup();
    srcImg.load("A.jpg");

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
    gui.addToggle("mask", false);
    gui.addSlider("threshold",0,1,0.5);
    gui.addSlider("gain",0.,10.,1);

    gui.addHeader("Dynamic Mapping");
    gui.addFooter();

    gui.on2dPadEvent(this, &ofApp::on2dPadEvent);
    gui.onSliderEvent(this, &ofApp::onSliderEvent);
    gui.onToggleEvent(this, &ofApp::onToggleEvent);

    reload();

    setupShader();
}

void ofApp::setupShader(){
#ifdef TARGET_OPENGLES
shader.load("shaders_gles/alphamask.vert","shaders_gles/alphamask.frag");
#else
if(ofIsGLProgrammableRenderer()){
    ofLogNotice("setupShader") << "using GLProgrammableRenderer " << endl;
    string vertex = "#version 150\n\
    \n\
    uniform mat4 projectionMatrix;\n\
    uniform mat4 modelViewMatrix;\n\
    uniform mat4 modelViewProjectionMatrix;\n\
    \n\
    \n\
    in vec4  position;\n\
    in vec2  texcoord;\n\
    \n\
    out vec2 texCoordVarying;\n\
    \n\
    void main()\n\
    {\n\
        texCoordVarying = texcoord;\
        gl_Position = modelViewProjectionMatrix * position;\n\
    }";
    string fragment = "#version 150\n\
    \n\
    uniform sampler2DRect tex0;\
    uniform sampler2DRect maskTex;\
    in vec2 texCoordVarying;\n\
    \
    out vec4 fragColor;\n\
    void main (void){\
    vec2 pos = texCoordVarying;\
    \
    vec3 src = texture(tex0, pos).rgb;\
    float mask = texture(maskTex, pos).r;\
    \
    fragColor = vec4( src , mask);\
    }";
    shader.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
    shader.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
    shader.bindDefaults();
    shader.linkProgram();
}else{
    ofFile shaderFile;
    shaderFile.open("shader/mask.frag");
    // TODO move this to a file
    string shaderProgram = "#version 120\n \
    #extension GL_ARB_texture_rectangle : enable\n \
    \
    uniform sampler2DRect tex0;\
    uniform sampler2DRect maskTex;\
    uniform bool maskFlag;\
    uniform float gain;\
    uniform float threshold;\
    \
    void main (void){\
    vec2 pos = gl_TexCoord[0].st;\
    \
    vec3 src = texture2DRect(tex0, pos).rgb;\
    float mask = texture2DRect(maskTex, pos).r;\
    mask *= gain * step(threshold, mask);\
    \
    if (maskFlag){\
        gl_FragColor = vec4( src , mask);\
    } else {\
        gl_FragColor = vec4(mask,mask,mask,1.);\
    }\
    }";
    shader.setupShaderFromSource(GL_FRAGMENT_SHADER, shaderProgram);
            //shader.load("shader/mask");
    shader.linkProgram();
}
#endif
}

void ofApp::update()
{
    if(camera.grabVideo(curFrame)) {
        curFrame.update();
    }
}

void ofApp::draw()
{
    ofClear(0, 0, 0, 255);
    // If the camera isn't ready, the curFrame will be empty.
    if(!camera.isReady()) return;

    // Camera doesn't draw itself, curFrame does.

    fbo.begin();
    ofClear(0, 0, 0, 0);

    shader.begin();
    shader.setUniformTexture("maskTex", curFrame.getTexture(), 1 );
    shader.setUniform1i("maskFlag", mask);
    shader.setUniform1f("threshold", threshold);
    shader.setUniform1f("gain", gain);
    //ofLogNotice("shader values") << "gain : " << gain << " threshold : " << threshold;
    srcImg.draw(0,0);
    shader.end();

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

void ofApp::onToggleEvent(ofxDatGuiToggleEvent e){

    if (e.target->is("mask")) mask = e.checked;
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
