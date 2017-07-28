#include "dynamic_mapping.h"

int main()
{
  ofGLFWWindowSettings settings;

  settings.width = 1920;
  settings.height = 1200;
  settings.setPosition(ofPoint(3600,0));
  //settings.setPosition(ofPoint(1680,0));
  settings.decorated = false;
  //settings.windowMode = OF_FULLSCREEN;
/*
  settings.width = 950;
  settings.height = 600;
*/
  settings.title = "Dynamic Mapping";

  auto window = ofCreateWindow(settings);
  auto app = make_shared<dynamic_mapping>();
  ofRunApp(window, app);

  return ofRunMainLoop();
}
