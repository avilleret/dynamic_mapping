#version 120
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect tex0;
uniform sampler2DRect maskTex;
uniform bool maskFlag;
uniform float gain;
uniform float threshold;

void main (void){
    vec2 pos = gl_TexCoord[0].st;

    vec3 src = texture2DRect(tex0, pos).rgb;
    float mask = texture2DRect(maskTex, pos).r;
    mask *= gain;
    mask *= step(threshold, mask);

    if (maskFlag){
        gl_FragColor = vec4( src , mask);
    } else {
        gl_FragColor = vec4(mask,mask,mask,1.);
    }
}
