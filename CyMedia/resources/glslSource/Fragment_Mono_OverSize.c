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

vec4 calcRGBA(sampler2D texture, ivec2 texSize, ivec2 intTexCoord){
    int monoIndex = intTexCoord.y * texSize.x + intTexCoord.x;
    int RGBA_X = (intTexCoord.x / 4) + (intTexCoord.y % 2 * (nWidth / 4));
    int RGBA_Y = intTexCoord.y / 2;
    int RGBA_C = monoIndex % 4;
    float gray = texelFetch(texture, ivec2(RGBA_X, RGBA_Y), 0)[RGBA_C] * bitMul;
    return vec4(gray, gray, gray, 1.0);
}