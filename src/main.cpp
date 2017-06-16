#include "dynamic_mapping.h"

int main()
{
  ofGLFWWindowSettings settings;

  settings.width = 1400;
  settings.height = 1050;
  settings.setPosition(ofPoint(1920,0));
  settings.decorated = false;
  settings.title = "Dynamic Mapping";

  auto window = ofCreateWindow(settings);
  auto app = make_shared<dynamic_mapping>();
  ofRunApp(window, app);

  return ofRunMainLoop();
}
