#pragma once

#include <opencv2/stitching.hpp>

#include "ofxGem.h"
// #include "ofxCv.h"

class Pix_share : public ofxGem {

public:
  enum source {
    SOURCE_NONE,
    SOURCE_FGMASK,
    SOURCE_FLOW
  };

  Pix_share();

  void setup(std::string name);
  // void setPixels(cv::Mat mat);
  void setPixels(ofPixels pix);
  source getSource(){ return m_source; }
  void setSource(source s) { m_source = s; }

private:
  std::string m_name;
  source m_source;
};
