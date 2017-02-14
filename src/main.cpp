#include "ofApp.h"

int main()
{
  ofGLFWWindowSettings settings;

  settings.width = 1400;
  settings.height = 1050;
  settings.setPosition(ofPoint(1920,0));
  settings.decorated = false;

  auto window = ofCreateWindow(settings);
  auto app = make_shared<ofApp>();
  ofRunApp(window, app);

  return ofRunMainLoop();
}
