#include "dynamic_mapping.h"


void Blob::draw(){
    ofPushMatrix();
    ofPushStyle();
    ofNoFill();
    ofDrawRectangle(bounding_box);
    string msg = ofToString(id);
    ofDrawBitmapString(msg, centroid.x + 4, centroid.y);
    // ofDrawCircle(centroid.x,centroid.y,2);
    ofScale(5, 5);
    ofDrawLine(0, 0, velocity.x, velocity.y);
    ofPopStyle();
    ofPopMatrix();
}

void dynamic_mapping::setup()
{
    ofSetLogLevel(OF_LOG_NOTICE);

    ofDirectory dir;
    dir.listDir("images/of_logos");
    dir.sort(); // in linux the file system doesn't return file lists ordered in alphabetical order

    //allocate the vector to have as many ofImages as files
    if( dir.size() ){
            images.assign(dir.size(), ofImage());
    }

    // you can now iterate through the files and load them into the ofImage vector
    for(int i = 0; i < (int)dir.size(); i++){
            if(i<images.size()) {
                // images[i].allocate(OF_IMAGE_COLOR_ALPHA);
                images[i].setImageType(OF_IMAGE_COLOR_ALPHA);
                images[i].load(dir.getPath(i));
            }
            i++;
    }

    texture.loadData(images[0].getPixels());

    int w = ofGetWidth();
    int h = ofGetHeight();
    int x = 0;
    int y = 0;

    fbo.allocate(w,h);

    texture.allocate(640, 480,OF_IMAGE_COLOR_ALPHA);
    fgmask.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR);
    fgmask.setColor(ofColor::white);
    pix_share.setup("/video_server");
    warper.setSourceRect(ofRectangle(0, 0, pix_share.getWidth(), pix_share.getHeight()));              // this is the source rectangle which is the size of the image and located at ( 0, 0 )
    warper.setTargetRect(ofRectangle(0,0,w,h));
    warper.setup();

    gui.addSlider("source width", 0, 1920, 640);
    gui.addSlider("source height", 0, 1080, 480);

    gui.addBreak();
    gui.addToggle("mask", false);
    gui.addSlider("threshold",0,10,0.5);
    gui.addSlider("gain",0.,10.,1);
    gui.addColorPicker("Clear color", ofColor::blue);

    gui.addHeader("Dynamic Mapping");
    gui.addFooter();

    gui.on2dPadEvent(this, &dynamic_mapping::on2dPadEvent);
    gui.onSliderEvent(this, &dynamic_mapping::onSliderEvent);
    gui.onToggleEvent(this, &dynamic_mapping::onToggleEvent);
    gui.onColorPickerEvent(this, &dynamic_mapping::onColorPickerEvent);
    gui.setAutoDraw(false);

    reload();

    // setupShader();
    shader.load("shader/mask");

    ofLogNotice("setup") << "load perlin shader" << std::endl;
    perlinShader.load("shader/perlin");
    // perlinShader.load("shader/noise");
    //  perlinShader.setupShaderFromFile(GL_FRAGMENT_SHADER,ofToDataPath("shader/perlin.frag"));
    //  perlinShader.linkProgram();
    ofLogNotice("setup") << "loading done";

    receiver.setup(3615);
    pd.setup(3616);
}

void dynamic_mapping::setupShader(){
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

void dynamic_mapping::update()
{
    if(receiver.hasWaitingMessages()) blobs.clear(); // clear only when new blos are received
    // check for waiting messages
    while(receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(m);
        if(m.getAddress() == "/b"){
            if (m.getNumArgs() == 10){
                Blob blob;
                int i=0;
                blob.id = m.getArgAsInt(i++);
                blob.centroid.x = m.getArgAsFloat(i++);
                blob.centroid.y = m.getArgAsFloat(i++);
                blob.area = m.getArgAsFloat(i++);
                blob.bounding_box.x = m.getArgAsFloat(i++);
                blob.bounding_box.y = m.getArgAsFloat(i++);
                blob.bounding_box.width = m.getArgAsFloat(i++);
                blob.bounding_box.height = m.getArgAsFloat(i++);
                blob.velocity.x = m.getArgAsFloat(i++);
                blob.velocity.y = m.getArgAsFloat(i++);
                blobs.push_back(blob);
            } else {
                ofLogError(__func__) << "wrong argument length: " << m.getNumArgs();
            }

        }
    }

    while(pd.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(m);
        if(m.getAddress() == "/a"){
            if (m.getNumArgs() == 3){
                alpha[0] = m.getArgAsInt(0);
                alpha[1] = m.getArgAsInt(1);
                alpha[2] = m.getArgAsInt(2);
            } else {
                ofLogError(__func__) << "wrong argument length: " << m.getNumArgs();
            }

        }
    }

    std::cout << "blob size: " << blobs.size() << std::endl;

    if (showGui){
        gui.update();
    }

    vector<ofPoint> src = warper.getSourcePoints();
    int width = src[2].x - src[0].x;
    int height = src[2].y - src[0].y;
    if (width != pix_share.getWidth() || height != pix_share.getHeight()){
        warper.setSourceRect(ofRectangle(0,0,pix_share.getWidth(), pix_share.getHeight()));
        fgmask.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);
        texture.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);
    }

    pix_share.update();
    cvmask=ofxCv::toCv(pix_share.getPixels());
    cvmask = ~cvmask;
    ofImage tmp;
    ofxCv::toOf(cvmask,tmp);
    fgmask.getPixels().setChannel(4,tmp);
    fgmask.update();
}

void dynamic_mapping::draw()
{
    ofClear(clearColor);

    ofMatrix4x4 mat = warper.getMatrix();

    ofPushMatrix();
    ofMultMatrix(mat);

    ofPushMatrix();
    ofPushStyle();
    ofScale(pix_share.getWidth(), pix_share.getHeight(),0.);

    for(int i = 0; i<blobs.size() && i<images.size(); i++){
      ofRectangle rect = blobs[i].bounding_box;
      ofSetColor(255, 255, 255, alpha[0]);
      images[i].draw(rect.x,rect.y,rect.width, rect.height);
    }
    ofPopStyle();
    ofPopMatrix();

  //  ofSetColor(ofColor::white);
//    ofEnableAlphaBlending();
    //pix_share.draw();
    fgmask.draw(0,0);
  //  ofEnableAlphaBlending();

    //fbo.draw(0, 0);

    ofPopMatrix();
/*
    perlinShader.begin();
    ofRect(0, 0, ofGetWidth(), ofGetHeight());
    perlinShader.end();
*/
    if (showGui){
        warper.draw();
        gui.draw();
    }
}

void dynamic_mapping::exit(){
}

void dynamic_mapping::on2dPadEvent(ofxDatGui2dPadEvent e){
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

void dynamic_mapping::onSliderEvent(ofxDatGuiSliderEvent e){

    if (e.target->is("threshold")) threshold = e.value;
    else if (e.target->is("gain")) gain = e.value;
}

void dynamic_mapping::onToggleEvent(ofxDatGuiToggleEvent e){

    if (e.target->is("mask")) mask = e.checked;
}

void dynamic_mapping::onColorPickerEvent(ofxDatGuiColorPickerEvent e){

    if (e.target->is("clear color")) clearColor = e.color;
}

void dynamic_mapping::keyPressed(ofKeyEventArgs& key)
{
    switch(key.key){
    case 'r': // reset warper;
    {
        int w = gui.getSlider("source width")->getValue();
        int h = gui.getSlider("source height")->getValue();
        int x = (ofGetWidth() - w) * 0.5;       // center on screen.
        int y = (ofGetHeight() - h) * 0.5;     // center on screen.

        warper.setSourceRect(ofRectangle(0, 0, w, h));
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

        warper.setTargetRect(ofRectangle(0,0,ofGetWidth(), ofGetHeight()));
        break;
    }
    case 'h':
        showGui = !showGui;
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

void dynamic_mapping::reload(){
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
