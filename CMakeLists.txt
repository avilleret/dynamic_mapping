project(ofApp)
set(APP ofApp)

cmake_minimum_required(VERSION 2.8)

include(../of/openFrameworks.cmake)
find_package( DC1394 REQUIRED )

ofxaddon(ofxLibdc)
ofxaddon(ofxHomography)
ofxaddon(ofxQuadWarp)
ofxaddon(ofxDatGui)

set(SOURCES
    src/main.cpp
    src/ofApp.cpp
)

add_executable(
    ${APP}
    MACOSX_BUNDLE
    ${SOURCES}
    ${OFXADDONS_SOURCES}
)

target_link_libraries(
    ${APP}
    ${OPENFRAMEWORKS_LIBRARIES}
    ${DC1394_LIBRARIES}
)

set_target_properties(
    ${APP} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
    "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
)

cotire(${APP})
