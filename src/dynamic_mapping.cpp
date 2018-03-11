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
  Display* display = ofGetX11Display();
  Window window = ofGetX11Window();

  Cursor invisibleCursor;
  Pixmap bitmapNoData;
  XColor black;
  static char noData[] = { 0,0,0,0,0,0,0,0 };
  black.red = black.green = black.blue = 0;

  bitmapNoData = XCreateBitmapFromData(display, window, noData, 8, 8);
  invisibleCursor = XCreatePixmapCursor(display, bitmapNoData, bitmapNoData,
                                       &black, &black, 0, 0);
  XDefineCursor(display,window, invisibleCursor);
  XFreeCursor(display, invisibleCursor);

  ofSetLogLevel(OF_LOG_NOTICE);
  ofSetVerticalSync(false);
  ofHideCursor();

  max_length = sqrt(ofGetHeight()*ofGetHeight() + ofGetWidth()*ofGetWidth());

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

  // setupShader();
  shader.load("shader/mask");
  if(ofIsGLProgrammableRenderer()){
    lineshader.load("shaders_gl3/noise");
  }else{
    lineshader.load("shaders/noise");
  }

  ossia.setup("OSCQuery", "dynamic_mapping", 6543, 8765);
  inputParam.setup(ossia.get_root_node(), "input");
  inputGain.setup(inputParam, "gain", 1., 0., 10.);
  inputThreshold.setup(inputParam, "threshold", 128, 0, 255);

  lineParam.setup(ossia.get_root_node(), "line");
  lineGap.setup(lineParam,"gap", 50, 0, 100);
  lineWidth.setup(lineParam,"width",10,1,100);
  lineRotation.setup(lineParam,"angle",0.,0.,360.);
  lineColor.setup(lineParam,"color", ofColor::white, ofColor(0.,0.,0.,0.), ofColor(255,255,255,255));
  lineOffset.setup(lineParam,"offset",ofVec2f(-100.,-250.),ofVec2f(-100,-100),ofVec2f(100,100));
  lineNoiseSpeed.setup(lineParam, "noise_speed", ofVec2f(10,10), ofVec2f(0.,0.), ofVec2f(100.,100.));
  lineNoiseAmount.setup(lineParam, "noise_amount", 0., 0., 100.);
  lineResolution.setup(lineParam, "resolution", 100, 2, 500);

  drawParam.setup(ossia.get_root_node(), "draw");
  drawMask.setup(drawParam, "mask",true);
  drawLines.setup(drawParam, "lines", true);
  drawInputImage.setup(drawParam, "input", false);
  drawBlobs.setup(drawParam, "blobs", true);

  /*
  for (int i=0;i<5;i++){
    blobColor[i].setup(ossia.get_root_node(), "color", ofFloatColor(0.,0.,0.,1.));
  }
  */

  connect_to_voxelstrack();

  setupGui();

  blobs.resize(5);

  reload();

  reload();
}

void dynamic_mapping::setupGui(){
  gui.addFRM();

  auto slider = gui.addSlider("window X offset", 0, 5000, 3600);
  slider->setPrecision(0);
  slider->onSliderEvent([this](ofxDatGuiSliderEvent e)
  {
    ofSetWindowPosition(e.value,0);
  });

  slider = gui.addSlider("window X size", 0, 5000, 1920);
  slider->setPrecision(0);
  slider->onSliderEvent([this](ofxDatGuiSliderEvent e)
  {
    ofSetWindowShape(e.value,ofGetWindowHeight());
  });

  slider = gui.addSlider("window Y size", 0, 5000, 1200);
  slider->setPrecision(0);
  slider->onSliderEvent([this](ofxDatGuiSliderEvent e)
  {
    ofSetWindowShape(ofGetWindowWidth(),e.value);
  });

//  gui.addSlider("source width", 0, 1920, 640);
//  gui.addSlider("source height", 0, 1080, 480);

//  gui.add2dPad("source rect position",ofRectangle(0,0,640,480));
//  gui.addSlider("source rect width",0,640,640);
//  gui.addSlider("source rect height",0,480,480);

  gui.addBreak();
  gui.addToggle("Draw lines", false);
  gui.addToggle("Draw blobs", false);
  gui.addToggle("Draw input image", false);
  gui.addToggle("mask", false);
  gui.addSlider("threshold",0,255,128);
  gui.addSlider("gain",0.,10.,1);
  gui.addColorPicker("Clear color", ofColor::blue);
  gui.addSlider("hline", 0, 1000);
  gui.addSlider("vline", 0, 1000);
  gui.addSlider("rline", 0, 360);

  auto tog = gui.addToggle("verbose");
  tog->onToggleEvent([this](ofxDatGuiToggleEvent e){
    if (e.checked)
      ofSetLogLevel(OF_LOG_VERBOSE);
    else
      ofSetLogLevel(OF_LOG_NOTICE);
  });

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
  std::string wsurl = "ws://127.0.0.1:5678";

  try{
    auto protocol = new ossia::oscquery::oscquery_mirror_protocol{wsurl};
    client_device = new ossia::net::generic_device{std::unique_ptr<ossia::net::protocol_base>(protocol), "voxelstrack"};

  } catch (const std::exception&  e) {
    ofLogError("ossia_client_connect") << e.what() << std::endl;
    return;
  }

  if (client_device) {
    std::cout << "connected to device " << client_device->get_name() << " on " << wsurl << std::endl;
    client_device->get_protocol().update(*client_device);
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

  if (client_device){
    // client_device->get_protocol().update(*client_device);
    int blobNum = 0;
    ossia::net::node_base* blobNode = ossia::net::find_node(*client_device, "blobNum");
    if (blobNode){
      ossia::value val = blobNode->get_address()->fetch_value();
      blobNum = ossia::MatchingType<int>::convertFromOssia(val);
    }
    int i = 0;
    for (i = 0; i < 5 && i < blobNum; i++ ){
      std::stringstream ss;
      ss << "/blob." << i;
      ossia::net::node_base* node = ossia::net::find_node(*client_device, ss.str());
      if (node) {
        auto bbnode = ossia::net::find_node(*node,"bounding_box");
        if (bbnode) {
          ossia::value val = bbnode->get_address()->fetch_value();
          ofVec4f bb = ossia::MatchingType<ofVec4f>::convertFromOssia(val);
          blobs[i].bounding_box = ofRectangle(bb.x, bb.y, bb.z, bb.w);
        }

        auto cnode = ossia::net::find_node(*node, "color");
        if (cnode) {
          ossia::value val = cnode->get_address()->fetch_value();
          blobs[i].color = ossia::MatchingType<ofFloatColor>::convertFromOssia(val);
        }
        ofLogVerbose(ss.str()) << "coords: " << blobs[i].bounding_box;
        ofLogVerbose(ss.str()) << "color: " << blobs[i].color.r << ";" << blobs[i].color.g << ";" << blobs[i].color.b << ";" << blobs[i].color.a;

      }
    }
    for (;i<blobs.size();i++){
      // reset bounding box on other blobs
      blobs[i].bounding_box = ofRectangle(0,0,0,0);
    }
  } else {
    if (ofGetElapsedTimeMillis() - lastTime > 1000.){
      connect_to_voxelstrack();
      lastTime = ofGetElapsedTimeMillis();
    }
  }

  if (showGui){
    gui.update();
  }

  /*
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
  */

  fgmask.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);
  fgmask.setColor(ofColor::black);
  // ofLogNotice("UPDATE") << "texture.allocate" ;
  // texture.allocate(pix_share.getWidth(), pix_share.getHeight(),OF_IMAGE_COLOR_ALPHA);

  pix_share.update();
  cvmask=ofxCv::toCv(pix_share.getPixels());
  cv::threshold(cvmask, cvmask, inputThreshold, 255, cv::THRESH_TOZERO);
  cvmask *= inputGain;
  cvmask = ~cvmask;
  ofImage tmp;
  ofxCv::toOf(cvmask,tmp);
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
    ofTranslate(lineOffset->x, lineOffset->y);

    ofSetColor(lineColor);

    float pos = 0;
    /*
    while(pos < max_length){
      ofDrawRectangle(pos,0,lineWidth,max_length);
      pos+=lineWidth + lineGap;
    }
    */


    /*
    if( dolineShader ){
      lineshader.begin();
      //we want to pass in some varrying values to animate our type / color
      lineshader.setUniform1f("timeValX", lineNoiseSpeed.get().x * ofGetElapsedTimef() * 0.1 );
      lineshader.setUniform1f("timeValY", lineNoiseSpeed.get().y * -ofGetElapsedTimef() * 0.1 );
      lineshader.setUniform1f("noiseamount", lineNoiseAmount);
    }
    */


    // FIXME : this shouldn't be necessary at all now
    pos = 0.;
    ofFill();
    //ofSetLineWidth(1);
    ofVec2f noiseTime(lineNoiseSpeed.get().x * ofGetElapsedTimef() * 0.01, lineNoiseSpeed.get().y * -ofGetElapsedTimef() * 0.01 );
    while(pos < max_length){
      ofPolyline line;
      line.addVertex(pos,0,0);
      line.addVertex(pos,max_length,0);
      //line.addVertex(pos+lineWidth,max_length,0);
      //line.addVertex(pos+lineWidth,0,0);
      //line.setClosed(true);
      line = line.getResampledByCount(lineResolution);
      // line.draw();
      ofFill();
      ofBeginShape();
      for (auto& pt : line.getVertices()){
        pt.x += lineNoiseAmount*ofNoise(pt+noiseTime);
        ofVertex(pt.x, pt.y);
      }

      for (int i = line.size(); i-- > 0;){
        ofPoint pt = line[i];
        ofVertex(pt.x+lineWidth,pt.y);
      }

      ofEndShape(OF_CLOSE);
      pos+=lineWidth + lineGap;
    }

    /*
    if( dolineShader ){
      lineshader.end();
    }
    */

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

    // ofLogNotice("dynamic_mapping") << "blobs size: " << blobs.size();

    ofEnableAlphaBlending();
    for(int i = 0; i<blobs.size() && i<10; i++){
      /*
      if (m_dist2luma != 0.) c.a = fmod(ofClamp(m_dist2luma * blobs[i].distance, -255., 255.)+255., 255.);
      blobnoisetime += blobnoisespeed;
      float noisevalue = blobcoloroffset + blobnoiseamount * std::max((ofNoise(blobnoisetime + blobs[i].age/1000.,i)-blobnoiseoffset)*(1-blobnoiseoffset),0.f);
      c.r *= noisevalue;
      c.g *= noisevalue;
      c.b *= noisevalue;
      c.a *= distanceColor.a;
      // ofLogNotice("1") << i << " age: " << blobs[i].age << " color: " << c << "\t noise: "<< noisevalue << " ddd " << ofNoise(blobs[i].age/1000.);
      */
      ofSetColor(blobs[i].color);
      ofDrawRectangle(blobs[i].bounding_box);
      // ofLogNotice("dynamic_mapping") << i << ":" << blobs[i].bounding_box;

      /*
      if ( i < noises.size() ){
        c = noiseColor;
        c *= noisevalue;
        c.a = noiseColor.a;
        if (m_dist2noise != 0.) c.a = fmod(ofClamp(m_dist2luma * blobs[i].distance, -255., 255.)+255., 255.);
        ofSetColor(c);
        noises[i].draw(rect.x,rect.y,rect.width, rect.height);
      }
      */

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

  if (drawMask){
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
    ofSetColor(ofColor::red);
    ofDrawCircle(mouseX, mouseY, 4);
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

  if (e.target->is("threshold")) inputThreshold.set(e.value);
  else if (e.target->is("gain")) inputGain.set(e.value);
  else if (e.target->is("source rect width")) {
    sourceRect.width = e.value;
    warper.setSourceRect(sourceRect);
  } else if (e.target->is("source rect height")) {
    sourceRect.height = e.value;
    warper.setSourceRect(sourceRect);
  }
}

void dynamic_mapping::onToggleEvent(ofxDatGuiToggleEvent e){

  if (e.target->is("mask")) drawMask.set(e.checked);
  else if (e.target->is("Draw lines")) drawLines.set(e.checked);
  else if (e.target->is("Draw blobs")) drawBlobs.set(e.checked);
  else if (e.target->is("Draw input image")) {
    drawInputImage.set(e.checked);
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
      settings.save("settings.xml",&gui);
      break;
    case 'l':
      reload();
      break;
    case OF_KEY_TAB:
    {
      ofLogNotice("Window positionX") << ofGetWindowPositionX();
      if ( ofGetWindowPositionX() <= 0){
        ofSetWindowPosition(3600,0);
        ofSetWindowShape(1920,1200);
      } else ofSetWindowPosition(0,0);
    }
    default:
      ;
  }
}

void dynamic_mapping::reload(){
  warper.load(); // reload last saved changes$
  settings.load("settings.xml", &gui);
  // warper.setSourceRect(sourceRect);
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
