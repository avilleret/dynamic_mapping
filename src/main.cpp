#include "ofApp.h"

int main()
{
  ofGLFWWindowSettings settings;

  settings.width = 1680;
  settings.height = 800;
  settings.setPosition(ofPoint(1520,0));
  settings.decorated = false;

  auto window = ofCreateWindow(settings);
  auto app = make_shared<ofApp>();
  ofRunApp(window, app);

  return ofRunMainLoop();
}
