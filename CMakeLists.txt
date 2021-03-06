project(dynamic_mapping)

cmake_minimum_required(VERSION 3.9)

include(../of/openFrameworks.cmake)
find_package( DC1394 REQUIRED )
find_package( OpenCV REQUIRED )

ofxaddon(ofxLibdc)
ofxaddon(ofxHomography)
ofxaddon(ofxQuadWarp)
ofxaddon(ofxDatGui)
ofxaddon(ofxGem)
ofxaddon(ofxCv)
ofxaddon(ofxXmlSettings)
ofxaddon(ofxOSSIA)

include_directories(${CMAKE_CURRENT_LIST_DIR}/ofxCv/libs/ofxCv/include)

set(SOURCES
    src/main.cpp
    src/dynamic_mapping.cpp
    src/Pix_share.cpp
)

add_executable(
    ${CMAKE_PROJECT_NAME}
    MACOSX_BUNDLE
    ${SOURCES}
    ${OFXADDONS_SOURCES}
)

target_link_libraries(
    ${CMAKE_PROJECT_NAME}
    ${OPENFRAMEWORKS_LIBRARIES}
    ${DC1394_LIBRARIES}
    ${OpenCV_LIBS}
    avahi-client
    avahi-common
    ${CMAKE_CURRENT_LIST_DIR}/ofxOSSIA/libs/ossia/lib/linux64/libossia.a    
)

set_target_properties(
    ${CMAKE_PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
    "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
)

cotire(${CMAKE_PROJECT_NAME})
