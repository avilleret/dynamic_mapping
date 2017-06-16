#include "dynamic_mapping.h"
#include <ossia/network/oscquery/oscquery_mirror.hpp>

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
  ofSetVerticalSync(false);
  ofHideCursor();

  sourceRect = ofRectangle(0,0,640,480);

  int w = ofGetWidth();
  int h = ofGetHeight();
  int x = 0;
  int y = 0;

  fbo.allocate(w,h);
  for (int i = 0; i<8; i++){
    ofImage img;
    img.allocate(128,128,OF_IMAGE_COLOR_ALPHA);
    noises.push_back(img);
  }
  // texture.allocate(640, 480,OF_IMAGE_COLOR_ALPHA);
  fgmask.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR);
  fgmask.setColor(ofColor::black);
  pix_share.setup("/video_server");
  warper.setSourceRect(sourceRect);              // this is the source rectangle which is the size of the image and located at ( 0, 0 )
  warper.setTargetRect(ofRectangle(0,0,w,h));
  warper.setup();

  reload();

  // setupShader();
  shader.load("shader/mask");
  if(ofIsGLProgrammableRenderer()){
    lineshader.load("shaders_gl3/noise");
  }else{
    lineshader.load("shaders/noise");
  }

  ossia.setup("OSCQuery", "dynamic_mapping", 6543, 8765);
  lineParam.setup(ossia.get_root_node(), "line");
  lineGap.setup(lineParam,"gap", 10, 0, 100);
  lineWidth.setup(lineParam,"width",10,1,100);
  lineRotation.setup(lineParam,"angle",0.,0.,360.);
  lineColor.setup(lineParam,"color", ofColor::white, ofColor(0.,0.,0.,0.), ofColor(255,255,255,255));

  connect_to_voxelstrack();

  setupGui();

  blobs.resize(8);
}

void dynamic_mapping::setupGui(){
  gui.addFRM();
  gui.addSlider("source width", 0, 1920, 640);
  gui.addSlider("source height", 0, 1080, 480);

  /*
    gui.add2dPad("top left", ofRectangle(0,0,w,h));
    gui.add2dPad("top right", ofRectangle(0,0,w,h));
    gui.add2dPad("bottom right", ofRectangle(0,0,w,h));
    gui.add2dPad("bottom left", ofRectangle(0,0,w,h));
    */

  /*
    gui.add2dPad("src tl", ofRectangle(0,0,640,480));
    gui.add2dPad("src tr", ofRectangle(0,0,640,480));
    gui.add2dPad("src br", ofRectangle(0,0,640,480));
    gui.add2dPad("src bl", ofRectangle(0,0,640,480));
    */

  gui.add2dPad("source rect position",ofRectangle(0,0,640,480));
  gui.addSlider("source rect width",0,640,640);
  gui.addSlider("source rect height",0,480,480);
  gui.addBreak();
  gui.addToggle("Draw lines", false);
  gui.addToggle("Draw blobs", false);
  gui.addToggle("Draw input image", false);
  gui.addToggle("mask", false);
  gui.addSlider("threshold",0,10,0.5);
  gui.addSlider("gain",0.,10.,1);
  gui.addColorPicker("Clear color", ofColor::blue);
  gui.addSlider("hline", 0, 1000);
  gui.addSlider("vline", 0, 1000);
  gui.addSlider("rline", 0, 360);

  gui.addHeader("Dynamic Mapping");
  gui.addFooter();

  gui.on2dPadEvent(this, &dynamic_mapping::on2dPadEvent);
  gui.onSliderEvent(this, &dynamic_mapping::onSliderEvent);
  gui.onToggleEvent(this, &dynamic_mapping::onToggleEvent);
  gui.onColorPickerEvent(this, &dynamic_mapping::onColorPickerEvent);
  gui.setAutoDraw(false);
}

void dynamic_mapping::connect_to_voxelstrack(){
  // Setup OSSIA client
  try{

    std::string wsurl = "ws://127.0.0.1:5678";
    auto protocol = new ossia::oscquery::oscquery_mirror_protocol{wsurl};
    client_device = new ossia::net::generic_device{std::unique_ptr<ossia::net::protocol_base>(protocol), "voxelstrack"};
    if (client_device) std::cout << "connected to device " << client_device->get_name() << " on " << wsurl << std::endl;

  } catch (const std::exception&  e) {
    ofLogError("ossia_client_connect") << e.what() << std::endl;
    return;
  }
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

  //    // ofLogNotice("UPDATE");
  //    if(receiver.hasWaitingMessages()) blobs.clear(); // clear only when new blos are received
  //    // check for waiting messages
  //    while(receiver.hasWaitingMessages()){
  //        // get the next message
  //        ofxOscMessage m;
  //        receiver.getNextMessage(m);
  //        if(m.getAddress() == "/b"){
  //            if (m.getNumArgs() >= 11){
  //                Blob blob;
  //                int i=0;
  //                blob.id = m.getArgAsInt(i++);
  //                blob.centroid.x = m.getArgAsFloat(i++);
  //                blob.centroid.y = m.getArgAsFloat(i++);
  //                blob.area = m.getArgAsFloat(i++);
  //                blob.bounding_box.x = m.getArgAsFloat(i++);
  //                blob.bounding_box.y = m.getArgAsFloat(i++);
  //                blob.bounding_box.width = m.getArgAsFloat(i++);
  //                blob.bounding_box.height = m.getArgAsFloat(i++);
  //                blob.velocity.x = m.getArgAsFloat(i++);
  //                blob.velocity.y = m.getArgAsFloat(i++);
  //                blob.distance = m.getArgAsFloat(i++);
  //                blob.age = m.getArgAsFloat(i++);
  //                blobs.push_back(blob);
  //            } else {
  //                ofLogError(__func__) << "wrong argument length: " << m.getNumArgs();
  //            }
  //        }
  //    }

  //    // std::cout << "blob size: " << blobs.size() << std::endl;
  //    while(pd.hasWaitingMessages()){
  //        // get the next message
  //        ofxOscMessage m;
  //        pd.getNextMessage(m);
  //        if ( m.getAddress() == "/blob/hsba" ){
  //            if (m.getNumArgs() == 5)  {
  //                    ofColor color = ofColor::fromHsb(m.getArgAsFloat(1),m.getArgAsFloat(2),m.getArgAsFloat(3),m.getArgAsFloat(4));
  //                    for (auto b : blobs){
  //                      if (b.id == m.getArgAsInt(0)) b.color = color;
  //                      continue;
  //                    }
  //            } else {
  //                ofLogError(__func__) << "wrong argument length: " << m.getNumArgs();
  //            }
  //            std::reverse(blobColor.begin(), blobColor.end());
  //            ofLogNotice("OSC") << "blobColor:";
  //            for ( auto b : blobs ){
  //                ofLogNotice("OSC") << b.color;
  //            }
  //        } else if (m.getAddress() == "/blob/dist/scale"){
  //          m_dist2luma = m.getArgAsFloat(0);
  //        }  else if (m.getAddress() == "/blob/noiseamount"){
  //          blobnoiseamount = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/noiseoffset"){
  //          blobnoiseoffset = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/noisespeed"){
  //          blobnoisespeed = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/coloroffset"){
  //          blobcoloroffset = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/dist/color"){
  //            if (m.getNumArgs() == 4)  {
  //                ofColor color = ofColor::fromHsb(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2),m.getArgAsFloat(3));
  //                distanceColor = color;
  //                ofLogNotice("OSC") << "distanceColor: " << distanceColor;
  //            }
  //        } else if (m.getAddress() == "/blob/noise/scale"){
  //          m_dist2noise = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/noise/speed"){
  //          noiseSpeed = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/noise/freq"){
  //          noiseFreq = m.getArgAsFloat(0);
  //        } else if (m.getAddress() == "/blob/noise/color"){
  //          if (m.getNumArgs() == 4)  {
  //            ofColor color = ofColor::fromHsb(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2),m.getArgAsFloat(3));
  //            noiseColor = color;
  //          }
  //        } else if (m.getAddress() == "/warping/src"){
  //          if (m.getNumArgs() == 8){
  //            vector<ofPoint> line(warper.getSourcePoints());
  //            int i =0;
  //            line[0].x=m.getArgAsFloat(i++);
  //            line[0].y=m.getArgAsFloat(i++);
  //            line[1].x=m.getArgAsFloat(i++);
  //            line[1].y=m.getArgAsFloat(i++);
  //            line[2].x=m.getArgAsFloat(i++);
  //            line[2].y=m.getArgAsFloat(i++);
  //            line[3].x=m.getArgAsFloat(i++);
  //            line[3].y=m.getArgAsFloat(i++);
  //            warper.setSourcePoints(line);
  //            ofLogNotice("OSC") << "update source points";
  //          } else {
  //            ofLogError(__func__) << "Message " << m.getAddress() << " wrong argument length: " << m.getNumArgs();
  //          }
  //        } else if (m.getAddress() == "/warping/dst"){
  //          if (m.getNumArgs() == 8){
  //            vector<ofPoint> line(warper.getTargetPoints());
  //            int i =0;
  //            line[0].x=m.getArgAsFloat(i++);
  //            line[0].y=m.getArgAsFloat(i++);
  //            line[1].x=m.getArgAsFloat(i++);
  //            line[1].y=m.getArgAsFloat(i++);
  //            line[2].x=m.getArgAsFloat(i++);
  //            line[2].y=m.getArgAsFloat(i++);
  //            line[3].x=m.getArgAsFloat(i++);
  //            line[3].y=m.getArgAsFloat(i++);
  //            warper.setTargetPoints(line);
  //            ofLogNotice("OSC") << "update target points";
  //          } else {
  //            ofLogError(__func__) << "Message " << m.getAddress() << " wrong argument length: " << m.getNumArgs();
  //          }
  //        }
  //        /*
  //         * else if ( m.getAddress() == "/line/hsba" ){
  //            if (m.getNumArgs() == 4)  {
  //                ofColor color = ofColor::fromHsb(m.getArgAsFloat(0),m.getArgAsFloat(1),m.getArgAsFloat(2),m.getArgAsFloat(3));
  //                lineColor = color;
  //            }
  //        } else if ( m.getAddress() == "/line/hvwn"){
  //            if (m.getNumArgs() == 5){
  //                hline = m.getArgAsFloat(0);
  //                vline = m.getArgAsFloat(1);
  //                lineWidth = m.getArgAsFloat(2);
  //                noisespeed = m.getArgAsFloat(3);
  //                noiseamount = m.getArgAsFloat(4);
  //            }
  //        } else if (m.getAddress() == "/line/sr"){
  //            if (m.getNumArgs() == 3){
  //                scaleline.x=m.getArgAsFloat(0);
  //                scaleline.y=m.getArgAsFloat(1);
  //                lineRotation=m.getArgAsFloat(2);
  //            }
  //        }
  //        */
  //    }
  if (showGui){
    gui.update();
  }

  for (int n = 0; n<noises.size() && n<blobs.size(); n++){
    ofFloatPixelsRef pix = noises[n].getPixels();
    float alpha = 255.;
    if (m_dist2noise != 0.) alpha = fmod(ofClamp(m_dist2noise * blobs[n].distance, -255., 255.)+255., 255.);
    int idx = 0;
    for (int i = 0; i<noises[n].getWidth(); i++){
      for (int j=0; j<noises[n].getHeight(); j++){
        float noise = ofNoise(noiseFreq*i,noiseFreq*j,n+noiseSpeed*ofGetElapsedTimef());
        pix[idx++] = noise;
        pix[idx++] = noise;
        pix[idx++] = noise;
        pix[idx++] = noiseColor.a;
        // noises[n].setColor(i,j,ofColor(noise));
        // noises[n].setColor(i,j,ofColor::white);
      }
    }
    noises[n].update();
  }

  ofSetLogLevel("UPDATE", OF_LOG_WARNING);
  ofLogNotice("UPDATE") << "fgmask.allocate" ;
  fgmask.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);
  ofLogNotice("UPDATE") << "fgmask.setColor" ;
  fgmask.setColor(ofColor::black);
  // ofLogNotice("UPDATE") << "texture.allocate" ;
  // texture.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);

  ofLogNotice("UPDATE") << "pix_share" ;
  pix_share.update();
  ofLogNotice("UPDATE") << "cvmask" ;
  cvmask=ofxCv::toCv(pix_share.getPixels());
  ofLogNotice("UPDATE") << "invert" ;
  cvmask = ~cvmask;
  ofImage tmp;
  ofLogNotice("UPDATE") << "toOf" ;
  ofxCv::toOf(cvmask,tmp);
  ofLogNotice("UPDATE") << "setChannel" ;
  fgmask.getPixels().setChannel(4,tmp);
  fgmask.update();
}

void dynamic_mapping::draw()
{

  // ofLogNotice("DRAW");
  ofClear(clearColor);

  // draw a solid black background
  //ofSetColor(ofColor(0,0,0,255));
  ofSetColor(clearColor);
  ofDrawRectangle(0,0,ofGetWidth(), ofGetHeight());

  if (drawLines){
    ofPushStyle();
    ofPushMatrix();
    // draw background lines
    ofScale(scaleline.x, scaleline.y ,1.);
    ofTranslate(ofGetWidth()/2.,ofGetHeight()/2.);
    ofRotateZ(lineRotation);

    ofTranslate(-ofGetWidth()/2.,-ofGetHeight()/2.);

    ofSetColor(lineColor);
    ofSetLineWidth(lineWidth);
    ofFill();
    //    if( dolineShader ){
    //        lineshader.begin();
    //            //we want to pass in some varrying values to animate our type / color
    //            lineshader.setUniform1f("timeValX", noisespeed * ofGetElapsedTimef() * 0.1 );
    //            lineshader.setUniform1f("timeValY", noisespeed * -ofGetElapsedTimef() * 0.18 );
    //            lineshader.setUniform1f("noiseamount", noiseamount);
    //    }
    for (int i = 0; i < lineSize->y; i++){
      int y = i*ofGetHeight()/lineSize->y;
      // TODO change that to ofDrawRectangle
      ofDrawLine(0,y,ofGetWidth(),y);
    }

    // FIXME : this shouldn't be necessary at all now
    for (int i = 0; i < lineSize->x; i++){
      float x = i*float(ofGetWidth())/float(lineSize->x);
      ofPolyline line;
      line.addVertex(x,-ofGetWidth(),0);
      line.addVertex(x,ofGetWidth(),0);
      //line.addVertex(x+wline,ofGetWidth(),0);
      //line.addVertex(x+wline,0,0);
      line.setClosed(true);
      line = line.getResampledByCount(500);
      line.draw();
    }

    //    if( dolineShader ){
    //        lineshader.end();
    //    }

    ofPopStyle();
    ofPopMatrix();
  }

  ofMatrix4x4 mat = warper.getMatrix();
  ofPushMatrix();
  ofMultMatrix(mat);

  if (drawBlobs){
    ofPushMatrix();
    ofPushStyle();
    ofScale(pix_share.getWidth(), pix_share.getHeight(),0.);

    ofEnableAlphaBlending();
    for(int i = 0; i<blobs.size(); i++){
      ofRectangle rect = blobs[i].bounding_box;
      ofColor c = blobs[i].color;
      if (m_dist2luma != 0.) c.a = fmod(ofClamp(m_dist2luma * blobs[i].distance, -255., 255.)+255., 255.);
      blobnoisetime += blobnoisespeed;
      float noisevalue = blobcoloroffset + blobnoiseamount * std::max((ofNoise(blobnoisetime + blobs[i].age/1000.,i)-blobnoiseoffset)*(1-blobnoiseoffset),0.f);
      c.r *= noisevalue;
      c.g *= noisevalue;
      c.b *= noisevalue;
      c.a *= distanceColor.a;
      // ofLogNotice("1") << i << " age: " << blobs[i].age << " color: " << c << "\t noise: "<< noisevalue << " ddd " << ofNoise(blobs[i].age/1000.);
      ofSetColor(c);
      ofDrawRectangle(rect);

      if ( i < noises.size() ){
        c = noiseColor;
        c *= noisevalue;
        c.a = noiseColor.a;
        if (m_dist2noise != 0.) c.a = fmod(ofClamp(m_dist2luma * blobs[i].distance, -255., 255.)+255., 255.);
        ofSetColor(c);
        noises[i].draw(rect.x,rect.y,rect.width, rect.height);
      }

      /*
      c = distanceColor;
      c = c * noisevalue;
      c.a = distanceColor.a;
      ofLogNotice("AGE") << i << " age: " << blobs[i].age << " color: " << c << "\t noise: "<< noisevalue << " ddd " << ofNoise(blobs[i].age/1000.);
      ofSetColor(c);
      ofDrawRectangle(rect);
*/
    }

    ofPopStyle();
    ofPopMatrix();
  }

  if (mask){
    // add some rectangle to mask the lines below the warper edge
    int s = 10000;
    int w = pix_share.getWidth();
    int h = pix_share.getHeight();
    ofSetColor(ofColor::black);
    ofDrawRectangle(-s,-s,2*s+w,s); // haut
    ofDrawRectangle(-s,h,2*s+w,s); // bas
    ofDrawRectangle(-s,-s,s,2*s+h); // droite
    ofDrawRectangle(w,-s,s,2*s+h); // gauche
    fgmask.draw(0,0);
  }
  //  ofEnableAlphaBlending();

  //fbo.draw(0, 0);

  ofPopMatrix();

  if (drawInputImage){
    //pix_share.draw(0,0,ofGetWidth(),ofGetHeight());
    //ofPushMatrix();
    //ofScale(ofGetWidth()/pix_share.getWidth(), ofGetHeight()/pix_share.getHeight());
    pix_share.draw();
    warper.draw();
  } else if (showGui){
    warper.draw();
  }

  if (showGui){
    gui.draw();
  }
}

void dynamic_mapping::exit(){
}

void dynamic_mapping::on2dPadEvent(ofxDatGui2dPadEvent e){
  vector<ofPoint> targetLine(warper.getTargetPoints());
  vector<ofPoint> srcLine(warper.getSourcePoints());

  if (e.target->is("top left")) {
    targetLine[0].x = e.x;
    targetLine[0].y = e.y;
  } else if (e.target->is("top right")) {
    targetLine[1].x = e.x;
    targetLine[1].y = e.y;
  } else if (e.target->is("bottom right")) {
    targetLine[2].x = e.x;
    targetLine[2].y = e.y;
  } else if (e.target->is("bottom left")) {
    targetLine[3].x = e.x;
    targetLine[3].y = e.y;
  } else if (e.target->is("src tl")) {
    ofLogNotice("__func__") << "set source top left to " << e.x << ";" << e.y;
    srcLine[0].x = e.x;
    srcLine[0].y = e.y;
  } else if (e.target->is("src tr")) {
    ofLogNotice("__func__") << "set source top right to " << e.x << ";" << e.y;
    srcLine[1].x = e.x;
    srcLine[1].y = e.y;
  } else if (e.target->is("src br")) {
    ofLogNotice("__func__") << "set source bottom right to " << e.x << ";" << e.y;
    srcLine[2].x = e.x;
    srcLine[2].y = e.y;
  } else if (e.target->is("src bl")) {
    ofLogNotice("__func__") << "set source bottom left to " << e.x << ";" << e.y;
    srcLine[3].x = e.x;
    srcLine[3].y = e.y;
  }

  warper.setTargetPoints(targetLine);
  warper.setSourcePoints(srcLine);

  if (e.target->is("source rect position")){
    sourceRect.x = e.x;
    sourceRect.y = e.y;
    ofLogNotice(__func__) << "setSourceRect";
    warper.setSourceRect(sourceRect);
  }

}

void dynamic_mapping::onSliderEvent(ofxDatGuiSliderEvent e){

  if (e.target->is("threshold")) threshold = e.value;
  else if (e.target->is("gain")) gain = e.value;
  else if (e.target->is("source rect width")) {
    sourceRect.width = e.value;
    warper.setSourceRect(sourceRect);
  } else if (e.target->is("source rect height")) {
    sourceRect.height = e.value;
    warper.setSourceRect(sourceRect);
  }
}

void dynamic_mapping::onToggleEvent(ofxDatGuiToggleEvent e){

  if (e.target->is("mask")) mask = e.checked;
  else if (e.target->is("Draw lines")) drawLines = e.checked;
  else if (e.target->is("Draw blobs")) drawBlobs = e.checked;
  else if (e.target->is("Draw input image")) {
    drawInputImage = e.checked;
    if (drawInputImage){
      warper.enableKeyboardShortcutsSrc();
      warper.enableMouseControlsSrc();
      warper.showSrc();
    } else {
      warper.enableKeyboardShortcuts();
      warper.enableMouseControls();
      warper.show();
    }
  }
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

      warper.setSourceRect(sourceRect);
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
    case OF_KEY_TAB:
    {
      ofLogNotice("Window positionX") << ofGetWindowPositionX();
      if ( ofGetWindowPositionX() <= 0){
        ofSetWindowPosition(1920,0);
        ofSetWindowShape(1920,1200);
      } else ofSetWindowPosition(0,0);
    }
    default:
      ;
  }
}

void dynamic_mapping::reload(){
  warper.load(); // reload last saved changes
  warper.setSourceRect(sourceRect);
  /*
    vector<ofPoin> src = warper.getSourceRect();
    ofxDatGui2dPad * pad = gui.get2dPad("top left");
    pad->setPoint(src[0]);
    pad = gui.get2dPad("top right");
    pad->setPoint(src[1]);
    pad = gui.get2dPad("bottom right");
    pad->setPoint(src[2]);
    pad = gui.get2dPad("bottom left");
    pad->setPoint(src[3]);
*/
}
