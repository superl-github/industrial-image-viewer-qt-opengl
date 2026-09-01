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

#define FOURCC_I422 15
#define FOURCC_YV12 16
#define FOURCC_I420 17

// YUV→RGB (BT.601)
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

    // Y值
    float yVal = texelFetch(texture, ivec2(x, y), 0).r;

    // 子采样因子
    int hSub = 2;
    int vSub = 1;
    if (colorType == FOURCC_I420 || colorType == FOURCC_YV12) {
        vSub = 2;
    }
    ivec2 uvCoord = ivec2(x / hSub, y / vSub);

    float uVal, vVal;
    // 判断U/V顺序
    if (colorType == FOURCC_I422 || colorType == FOURCC_I420) {
        uVal = texelFetch(textureU, uvCoord, 0).r;
        vVal = texelFetch(textureV, uvCoord, 0).r;
    } else { // YV16 / YV12
        vVal = texelFetch(textureU, uvCoord, 0).r; // textureU实际存储V
        uVal = texelFetch(textureV, uvCoord, 0).r; // textureV实际存储U
    }

    if (YUVMethod == 1) {
        return vec4(yVal, yVal, yVal, 1.0);
    } else {
        vec3 rgb = yuv2rgb(yVal, uVal, vVal);
        return vec4(rgb, 1.0);
    }
}