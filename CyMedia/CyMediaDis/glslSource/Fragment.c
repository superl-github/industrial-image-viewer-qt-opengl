#version 330 core

//======uniform======
uniform sampler2D texture;
uniform sampler2D texture_ColorMap;
uniform int nWidth;
uniform int nHeight;
uniform int nbits;
uniform int colorType;
uniform int colorMapIndex;
uniform float pixrange;
uniform int stretchType;
uniform int demosacFunc; // 0->None 1->BILINEAR 2->MALVA 3->AHD
uniform vec2 stretchPara;
uniform float zoomValue;

// ===== Bayer 去马赛克辅助 =====
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

vec3 malvaDemosaic(sampler2D tex, ivec2 coord, int pattern, ivec2 texSize) {
    ivec2 l = coord + ivec2(-1, 0), r = coord + ivec2(1, 0);
    ivec2 u = coord + ivec2(0, -1), d = coord + ivec2(0, 1);
    ivec2 ul = coord + ivec2(-1, -1), ur = coord + ivec2(1, -1);
    ivec2 dl = coord + ivec2(-1, 1), dr = coord + ivec2(1, 1);

    int cur = colorAt(coord, pattern);
    float R, G, B;

    if (cur == 1) {
        G = getBayerRaw(tex, coord, texSize);
        float Rsum = 0.0, Bsum = 0.0;
        int Rcnt = 0, Bcnt = 0;
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
    } else {
        float G_h = (getBayerRaw(tex, l, texSize) + getBayerRaw(tex, r, texSize)) * 0.5;
        float G_v = (getBayerRaw(tex, u, texSize) + getBayerRaw(tex, d, texSize)) * 0.5;
        float grad_h = abs(getBayerRaw(tex, l, texSize) - getBayerRaw(tex, r, texSize));
        float grad_v = abs(getBayerRaw(tex, u, texSize) - getBayerRaw(tex, d, texSize));
        G = (grad_h < grad_v) ? G_h : G_v;

        if (cur == 0) {
            R = getBayerRaw(tex, coord, texSize);
            B = (getBayerRaw(tex, ul, texSize) + getBayerRaw(tex, ur, texSize) +
                 getBayerRaw(tex, dl, texSize) + getBayerRaw(tex, dr, texSize)) * 0.25;
        } else { // cur == 2
            B = getBayerRaw(tex, coord, texSize);
            R = (getBayerRaw(tex, ul, texSize) + getBayerRaw(tex, ur, texSize) +
                 getBayerRaw(tex, dl, texSize) + getBayerRaw(tex, dr, texSize)) * 0.25;
        }
    }
    return vec3(R, G, B);
}


//======in======
in vec2 TexCoord;

//======out======
out vec4 fragColor;

// type: 1->Gray 2->HSV 3->Lab (L*)
// value: Input pixel color (typically RGBA, but only RGB part matters for conversion)
// para: vec2 containing (K, C) for the stretching operation (new_value = K * old_value + C)
vec4 stretchPixel(int type, vec4 value, vec2 para);

void main() {
    //===== 计算坐标 =====
    ivec2 texSize = textureSize(texture, 0);
    ivec2 intTexCoord = clamp(ivec2(TexCoord * vec2(texSize)), ivec2(0), ivec2(texSize) - 1);

    //===== 原始像素值 =====
    vec4 rgba = texelFetch(texture, intTexCoord, 0);

    //===== 计算像素值 =====
    float maxPixelValue = pixrange - 1.0;
    if (colorType == 0) {//Moon
        rgba.g = rgba.r;
        rgba.b = rgba.r;
		rgba.a = 1.0;
    }
    else if (colorType == 11) {//MONO_OVERSIZE
        int monoIndex = intTexCoord.y * texSize.x + intTexCoord.x;
        int RGBA_X = (intTexCoord.x / 4) + (intTexCoord.y % 2 * (nWidth / 4));
        int RGBA_Y = intTexCoord.y / 2;
        int RGBA_C = monoIndex % 4;
        rgba.r = texelFetch(texture, ivec2(RGBA_X, RGBA_Y), 0)[RGBA_C];
        rgba.g = rgba.r;
        rgba.b = rgba.r;
        rgba.a = 1.0;
    }
    else if (colorType >= 1 && colorType <= 4) {
        int pattern = colorType - 1; // 1->RGGB, 2->GRBG, 3->BGGR, 4->GBRG
        vec3 rgb;
        if (demosacFunc == 1) {
            rgb = bilinearDemosaic(texture, intTexCoord, pattern, texSize);
        } else if (demosacFunc == 2) {
            rgb = malvaDemosaic(texture, intTexCoord, pattern, texSize);
        } else if (demosacFunc == 3) {
            // AHD 未实现，回退双线性
            rgb = bilinearDemosaic(texture, intTexCoord, pattern, texSize);
        } else {
            // 无插值，直接取 R 通道作为灰度
            float v = texelFetch(texture, intTexCoord, 0).r;
            rgb = vec3(v);
        }
        rgba.rgb = rgb;
        rgba.a = 1.0;
    }
    

    //===== 位深处理 =====
    float colorScale = 1.0;
    float trueMaxPixelValue = 1.0;
    if (nbits > 8 && nbits < 16) {
        colorScale = 65536.0 / pixrange;
        trueMaxPixelValue = pixrange / 65536.0;
        rgba.r = rgba.r * colorScale; 
        rgba.g = rgba.g * colorScale;
        rgba.b = rgba.b * colorScale;
    }
    else if (nbits > 16 && nbits < 32) {
        colorScale = 4294967296.0 / pixrange;
        trueMaxPixelValue = maxPixelValue / 4294967296.0;
        rgba.r = rgba.r * colorScale; 
        rgba.g = rgba.g * colorScale;
        rgba.b = rgba.b * colorScale;
    }

    //===== 灰度拉伸 =====
    rgba = stretchPixel(stretchType, rgba, stretchPara);

    //===== 边际修正 =====
    rgba.r = clamp(rgba.r, 0.0, 1.0);
    rgba.g = clamp(rgba.g, 0.0, 1.0);
    rgba.b = clamp(rgba.b, 0.0, 1.0);
    
    //===== 色彩映射 =====
    if (colorMapIndex > 0) {
        float disValue = rgba.r * 0.299 + rgba.g * 0.587 + rgba.b * 0.114;
        rgba.r = texelFetch(texture_ColorMap, ivec2(int(disValue * 255.0), 0), 0).r;
        rgba.g = texelFetch(texture_ColorMap, ivec2(int(disValue * 255.0), 1), 0).r;
        rgba.b = texelFetch(texture_ColorMap, ivec2(int(disValue * 255.0), 2), 0).r;
    }
    
    //===== Border Drawing =====
    const float MIN_ZOOM_FOR_BORDER = 10.0;
    const float BORDER_WIDTH_IN_SCREEN_PIXELS = 1.0;
    const float BORDER_MIX_FACTOR = 0.25;
    bool drawBorder = false;
    if (zoomValue >= MIN_ZOOM_FOR_BORDER) {
        //// Convert to texel coordinates
        vec2 texelCoord = TexCoord * texSize;
        vec2 localFrac = fract(texelCoord);//[0,1) within current pixel
        //Calculate border width in texel space
        float borderInTexels = BORDER_WIDTH_IN_SCREEN_PIXELS / zoomValue;
        //Draw border near left edge or top edge
        drawBorder = (localFrac.x < borderInTexels) || (localFrac.y < borderInTexels);
    }
    if (drawBorder) {
        float luminance = rgba.r * 0.2126 + rgba.g * 0.7152 + rgba.b * 0.722;// Standard luminance weights
        vec4 borderColor = (luminance > 0.5) ? vec4(0.0) : vec4(1.0);
        borderColor.a = 1.0;
        rgba = mix(rgba, borderColor, BORDER_MIX_FACTOR);
    }

    fragColor = rgba;
};




// Helper function to clamp values explicitly in the fragment shader context
float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}
vec3 clampRGB(vec3 rgb) {
    return vec3(saturate(rgb.r), saturate(rgb.g), saturate(rgb.b));
}
// RGB <-> HSV
vec3 rgb2hsv(vec3 rgb) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(rgb.bg, K.wz), vec4(rgb.gb, K.xy), step(rgb.b, rgb.g));
    vec4 q = mix(vec4(p.xyw, rgb.r), vec4(rgb.r, p.yzx), step(p.x, rgb.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}
vec3 hsv2rgb(vec3 hsv) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(hsv.xxx + K.xyz) * 6.0 - K.www);
    return hsv.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), hsv.y);
}

//RGB <-> LAB
vec3 srgb2linear(vec3 srgb) {
    // 阈值 0.04045 对应 sRGB 规范
    vec3 lt = step(0.04045, srgb);
    return mix(
        srgb / 12.92,
        pow((srgb + 0.055) / 1.055, vec3(2.4)),
        lt
    );
}
vec3 linear2srgb(vec3 linear) {
    // 阈值 0.0031308 = (0.04045+0.055)/1.055^2.4
    vec3 lt = step(0.0031308, linear);
    return mix(
        12.92 * linear,
        1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055,
        lt
    );
}

vec3 rgb2lab(vec3 rgb) {
    // 1. sRGB → 线性 RGB
    vec3 lin = srgb2linear(rgb);
    
    // 2. 线性 RGB → XYZ (D65, 使用点积避免矩阵歧义)
    float X = dot(lin, vec3(0.4124564, 0.3575761, 0.1804375));
    float Y = dot(lin, vec3(0.2126729, 0.7151522, 0.0721750));
    float Z = dot(lin, vec3(0.0193339, 0.1191920, 0.9503041));
    
    // 3. XYZ 归一化到 D65 白点 (Xn=0.95047, Yn=1.0, Zn=1.08883)
    vec3 xyzn = vec3(X / 0.95047, Y, Z / 1.08883);
    
    // 4. 应用 f(t) 函数 (阈值 (6/29)^3 ≈ 0.008856)
    vec3 mask = step(0.008856, xyzn);
    vec3 fxyz = mix(
        7.787 * xyzn + (16.0 / 116.0), // 线性段
        pow(xyzn, vec3(1.0 / 3.0)),     // 非线性段
        mask
    );
    
    // 5. 计算 Lab
    float L = 116.0 * fxyz.y - 16.0;
    float a = 500.0 * (fxyz.x - fxyz.y);
    float b = 200.0 * (fxyz.y - fxyz.z);
    
    return vec3(L, a, b); // L∈[0,100], a/b∈[-128,127]（理论范围）
}
vec3 lab2rgb(vec3 lab){
    float L = lab.x;
    float a = lab.y;
    float b = lab.z;
    
    // 1. Lab → f(X/Xn), f(Y/Yn), f(Z/Zn)
    float fy = (L + 16.0) / 116.0;
    float fx = fy + a / 500.0;
    float fz = fy - b / 200.0;
    vec3 fvals = vec3(fx, fy, fz);
    
    // 2. 逆 f(t) 函数 (阈值 6/29 ≈ 0.20689655)
    vec3 mask = step(6.0 / 29.0, fvals);
    vec3 xyzn = mix(
        (fvals - (16.0 / 116.0)) / 7.787, // 线性段逆变换
        fvals * fvals * fvals,             // 非线性段逆变换
        mask
    );
    
    // 3. 乘回 D65 白点
    vec3 xyz = xyzn * vec3(0.95047, 1.0, 1.08883);
    
    // 4. XYZ → 线性 RGB (使用点积)
    vec3 lin;
    lin.r = dot(xyz, vec3( 3.2404542, -1.5371385, -0.4985314));
    lin.g = dot(xyz, vec3(-0.9692660,  1.8760108,  0.0415560));
    lin.b = dot(xyz, vec3( 0.0556434, -0.2040259,  1.0572252));
    
    // 5. 线性 RGB → sRGB
    return linear2srgb(lin);
}


vec4 stretchPixel(int type, vec4 value, vec2 para) {
    vec4 tempRGB = value;
    vec3 color_rgb = tempRGB.rgb;

    if (type == 1) { // Gray
        float disValue = color_rgb.r * 0.299 + color_rgb.g * 0.587 + color_rgb.b * 0.114;
        float stretched_lum = para.x * disValue + para.y;
        stretched_lum = saturate(stretched_lum);
        tempRGB.rgb = vec3(stretched_lum, stretched_lum, stretched_lum);
    }
    else if (type == 2) { // HSV
        vec3 hsv = rgb2hsv(color_rgb);
        float stretched_v = para.x * hsv.z + para.y;
        stretched_v = saturate(stretched_v);
        vec3 stretched_hsv = vec3(hsv.xy, stretched_v);
        vec3 stretched_rgb = hsv2rgb(stretched_hsv);
        tempRGB.rgb = clampRGB(stretched_rgb);
    }
    else if (type == 3) { // Lab
        vec3 lab = rgb2lab(color_rgb);
        //LAB->L:0~100
        float normalizedL = lab.x / 100.0;
        float stretched_l = para.x * normalizedL + para.y;
        stretched_l = saturate(stretched_l);
        float finalL = stretched_l * 100.0; //
        vec3 stretched_lab = vec3(finalL, lab.y, lab.z);
        vec3 stretched_rgb = lab2rgb(stretched_lab);
        tempRGB.rgb = clampRGB(stretched_rgb);
    }
    return tempRGB;
}

float remap(float value, float inMin, float inMax, float outMin, float outMax) {
    return (value - inMin) / (inMax - inMin) * (outMax - outMin) + outMin;
}
