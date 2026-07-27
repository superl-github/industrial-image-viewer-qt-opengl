// //======uniform======
// uniform sampler2D texture;
// uniform sampler2D texture_ColorMap;
// uniform int nWidth;
// uniform int nHeight;
// uniform int nbits;
// uniform int colorType;
// uniform int bayerPatter;
// uniform int colorMapIndex;
// uniform float pixrange;
// uniform int stretchType;
// uniform int demosacFunc; // 0->None 1->BILINEAR 2->MALVA 3->AHD
// uniform int YUVMethod;  //0->Normal 1->onlyY
// uniform vec2 stretchPara;
// uniform float zoomValue;
// //======in======
// in vec2 TexCoord;
// //======out======
// out vec4 fragColor;

#define FOURCC_NV12 18

vec3 yuv2rgb(float y, float u, float v) {
    float Y = y;
    float U = u - 0.5;
    float V = v - 0.5;
    float R = Y + 1.402 * V;
    float G = Y - 0.344 * U - 0.714 * V;
    float B = Y + 1.772 * U;
    return clamp(vec3(R, G, B), 0.0, 1.0);
}

vec4 calcRGBA(sampler2D texture, ivec2 texSize, ivec2 intTexCoord) {
    int x = intTexCoord.x;
    int y = intTexCoord.y;

    float yVal = texelFetch(texture, ivec2(x, y), 0).r;

    ivec2 uvCoord = ivec2(x / 2, y);   // 水平子采样2，垂直无子采样
    vec2 uv = texelFetch(textureU, uvCoord, 0).rg;

    float uVal, vVal;
    if (colorType == FOURCC_NV12) {
        uVal = uv.r;
        vVal = uv.g;
    } else { // NV21
        uVal = uv.g;
        vVal = uv.r;
    }

    if (YUVMethod == 1) {
        return vec4(yVal, yVal, yVal, 1.0);
    } else {
        vec3 rgb = yuv2rgb(yVal, uVal, vVal);
        return vec4(rgb, 1.0);
    }
}