#include "Pix_share.h"

Pix_share :: Pix_share() :
  m_source(Pix_share::SOURCE_FGMASK)
{}

void Pix_share :: setup(std::string name){

  std::string filename = "/tmp/pix_share_share_mem_id-";
  for (char& c : name) {
    // remove some invalid characters from the string
    if (( c >= '0' && c <= '9'  ) || ( c >= 'A' && c <= '~' )) filename +=c;
  }
  ofLogNotice("Pix_share") << filename;

  cout << "pix_share filename " << filename << endl;
  ifstream addrif(filename);

  int err=-1, count=0;
  float addr = 0;

  m_name = name;

  while(err!=0 && count < 10){
    std::string line;

    // check if there is already a segment with the same name
    if (addrif.is_open() && getline (addrif,line))
    {
      addr = std::stoi (line);
      addrif.close();
      err = ofxGem::setup(addr, 2000, 1500, 1);
    } else {
      // create a new segement with random address if needed...
      addr = rand() % 10000;
      err = ofxGem::setup(addr, 2000, 1500, 1);

      if ( err == 0 ) {
        // and save the address in a file
        ofstream addrof (filename);
        addrof << addr << ";" << endl;
        addrof.close();
      }
    }
    count++;
  }

  if ( err!= 0 ) ofLogError("Pix_share") << "can't get shared memory segment";
  else ofLogVerbose("Pix_share") << "shared memory segment " << name << " successfully created at " << addr;
}

/*
void Pix_share :: setPixels(cv::Mat mat){
  ofPixels pix;
  ofxCv::toOf(mat, pix);
  m_pix_share.setPixels(pix);
}
*/

void Pix_share :: setPixels(ofPixels pix){
  ofxGem::setPixels(pix);
}
