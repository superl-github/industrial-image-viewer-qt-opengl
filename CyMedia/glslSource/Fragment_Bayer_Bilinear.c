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

#define BAYER_RGGB 0
#define BAYER_GRBG 1
#define BAYER_BGGR 2
#define BAYER_GBRG 3

float getBayerRaw(sampler2D tex, ivec2 coord, ivec2 texSize) {
    coord = clamp(coord, ivec2(0), texSize - 1);
    return texelFetch(tex, coord, 0).r;
}

int colorAt(ivec2 coord, int pattern) {
    int r = coord.y & 1;
    int c = coord.x & 1;
    if (pattern == BAYER_RGGB) {
        if (r == 0 && c == 0) return 0;
        if (r == 0 && c == 1) return 1;
        if (r == 1 && c == 0) return 1;
        if (r == 1 && c == 1) return 2;
    } else if (pattern == BAYER_GRBG) {
        if (r == 0 && c == 0) return 1;
        if (r == 0 && c == 1) return 0;
        if (r == 1 && c == 0) return 2;
        if (r == 1 && c == 1) return 1;
    } else if (pattern == BAYER_BGGR) {
        if (r == 0 && c == 0) return 2;
        if (r == 0 && c == 1) return 1;
        if (r == 1 && c == 0) return 1;
        if (r == 1 && c == 1) return 0;
    } else { // GBRG
        if (r == 0 && c == 0) return 1;
        if (r == 0 && c == 1) return 2;
        if (r == 1 && c == 0) return 0;
        if (r == 1 && c == 1) return 1;
    }
    return 1;
}

vec3 bilinearDemosaic(sampler2D tex, ivec2 coord, int pattern, ivec2 texSize) {
    ivec2 l = coord + ivec2(-1, 0), r = coord + ivec2(1, 0);
    ivec2 u = coord + ivec2(0, -1), d = coord + ivec2(0, 1);
    ivec2 ul = coord + ivec2(-1, -1), ur = coord + ivec2(1, -1);
    ivec2 dl = coord + ivec2(-1, 1), dr = coord + ivec2(1, 1);

    int cur = colorAt(coord, pattern);
    float R, G, B;

    if (cur == 0) {
        R = getBayerRaw(tex, coord, texSize);
        G = (getBayerRaw(tex, u, texSize) + getBayerRaw(tex, d, texSize) +
             getBayerRaw(tex, l, texSize) + getBayerRaw(tex, r, texSize)) / 4.0;
        B = (getBayerRaw(tex, ul, texSize) + getBayerRaw(tex, ur, texSize) +
             getBayerRaw(tex, dl, texSize) + getBayerRaw(tex, dr, texSize)) / 4.0;
    } else if (cur == 2) {
        B = getBayerRaw(tex, coord, texSize);
        G = (getBayerRaw(tex, u, texSize) + getBayerRaw(tex, d, texSize) +
             getBayerRaw(tex, l, texSize) + getBayerRaw(tex, r, texSize)) / 4.0;
        R = (getBayerRaw(tex, ul, texSize) + getBayerRaw(tex, ur, texSize) +
             getBayerRaw(tex, dl, texSize) + getBayerRaw(tex, dr, texSize)) / 4.0;
    } else { // G 位置
        G = getBayerRaw(tex, coord, texSize);
        float Rsum = 0.0, Bsum = 0.0;
        int Rcnt = 0, Bcnt = 0;
        // 手动处理四个邻居
        int c;
        float val;
        c = colorAt(l, pattern); val = getBayerRaw(tex, l, texSize);
        if (c == 0) { Rsum += val; Rcnt++; } else if (c == 2) { Bsum += val; Bcnt++; }
        c = colorAt(r, pattern); val = getBayerRaw(tex, r, texSize);
        if (c == 0) { Rsum += val; Rcnt++; } else if (c == 2) { Bsum += val; Bcnt++; }
        c = colorAt(u, pattern); val = getBayerRaw(tex, u, texSize);
        if (c == 0) { Rsum += val; Rcnt++; } else if (c == 2) { Bsum += val; Bcnt++; }
        c = colorAt(d, pattern); val = getBayerRaw(tex, d, texSize);
        if (c == 0) { Rsum += val; Rcnt++; } else if (c == 2) { Bsum += val; Bcnt++; }
        R = (Rcnt > 0) ? Rsum / float(Rcnt) : 0.0;
        B = (Bcnt > 0) ? Bsum / float(Bcnt) : 0.0;
    }
    return vec3(R, G, B);
}

vec4 calcRGBA(sampler2D texture, ivec2 texSize, ivec2 intTexCoord) {
    return vec4(bilinearDemosaic(texture, intTexCoord, bayerPatter, texSize) * bitMul, 1.0);
}