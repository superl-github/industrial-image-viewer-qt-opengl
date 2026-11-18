// //======uniform======
// uniform sampler2D texture;
// uniform sampler2D texture_ColorMap;
// uniform int nWidth;
// uniform int nHeight;
// uniform int nbits;
// uniform int colorType;
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

// 超过最大纹理尺寸的灰度图：
//   原始数据被当成一维字节流，每 4 个像素打包进 1 个 RGBA texel
//   任意宽高均可（不要求 2/4 的倍数），容量上限 4 * maxTexSize^2 像素
vec4 calcRGBA(sampler2D texture, ivec2 texSize, ivec2 intTexCoord){
    // 由归一化 UV 恢复源图像素坐标（TexCoord 已在顶点端缩放到 [0,1]）
    ivec2 origSize = ivec2(nWidth, nHeight);
    ivec2 srcCoord = ivec2(TexCoord * vec2(origSize));
    srcCoord = clamp(srcCoord, ivec2(0), origSize - 1);

    // 线性像素索引 -> texel 索引 + channel
    int idx      = srcCoord.y * nWidth + srcCoord.x;
    int texelIdx = idx >> 2;   // / 4
    int channel  = idx & 3;    // % 4

    ivec2 tSize  = textureSize(texture, 0);
    ivec2 tcoord = ivec2(texelIdx % tSize.x, texelIdx / tSize.x);

    float gray = texelFetch(texture, tcoord, 0)[channel] * bitMul;
    return vec4(gray, gray, gray, 1.0);
}