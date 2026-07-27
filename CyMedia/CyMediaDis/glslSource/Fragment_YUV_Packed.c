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

#define FOURCC_YUY2 12
#define FOURCC_YVYU 13

//YUV转RGB (BT.601)
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
    // 纹理宽度为 2*nWidth，每个像素的Y在偶数索引位置
    int baseX = x * 2;
    float yVal = texelFetch(texture, ivec2(baseX, y), 0).r;
    float uVal, vVal;
    // 根据格式和奇偶性提取U/V
    if (colorType == FOURCC_YUY2) {
        if ((x & 1) == 0) {                     // 偶数像素
            uVal = texelFetch(texture, ivec2(baseX + 1, y), 0).r;
            vVal = texelFetch(texture, ivec2(baseX + 3, y), 0).r;
        } else {                                // 奇数像素
            uVal = texelFetch(texture, ivec2(baseX - 1, y), 0).r;
            vVal = texelFetch(texture, ivec2(baseX + 1, y), 0).r;
        }
    } else if (colorType == FOURCC_YVYU) {
        if ((x & 1) == 0) {
            vVal = texelFetch(texture, ivec2(baseX + 1, y), 0).r;
            uVal = texelFetch(texture, ivec2(baseX + 3, y), 0).r;
        } else {
            vVal = texelFetch(texture, ivec2(baseX - 1, y), 0).r;
            uVal = texelFetch(texture, ivec2(baseX + 1, y), 0).r;
        }
    }

    if (YUVMethod == 1) {
        return vec4(yVal, yVal, yVal, 1.0);
    } else {
        vec3 rgb = yuv2rgb(yVal, uVal, vVal);
        return vec4(rgb, 1.0);
    }
}