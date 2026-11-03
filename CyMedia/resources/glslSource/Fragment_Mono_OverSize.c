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

//超过最大尺寸的图像以RGBA，宽高各除以2的形式传入文理，这里解析回原来的图像
vec4 calcRGBA(sampler2D texture, ivec2 texSize, ivec2 intTexCoord){
    // 1. 根据 UV 和 纹理尺寸 计算浮点像素坐标
    vec2 floatCoord = TexCoord * vec2(texSize); 
    
    // 2. 显式转换为整型坐标
    ivec2 coord = ivec2(floatCoord); 
    
    // 3. 将坐标限制在原始图像的实际范围内 (nWidth x nHeight)
    ivec2 origSize = ivec2(nWidth, nHeight);
    coord = clamp(coord, ivec2(0), origSize - 1);

    // 4. 计算实际的打包坐标
    int x = coord.x;
    int y = coord.y;
    
    int monoIndex = y * nWidth + x; 
    int RGBA_X = (x / 4) + ((y % 2) * (nWidth / 4));
    int RGBA_Y = y / 2;
    int RGBA_C = monoIndex % 4;

    // 5. 采样并还原
    float gray = texelFetch(texture, ivec2(RGBA_X, RGBA_Y), 0)[RGBA_C] * bitMul;
    return vec4(gray, gray, gray, 1.0);
}