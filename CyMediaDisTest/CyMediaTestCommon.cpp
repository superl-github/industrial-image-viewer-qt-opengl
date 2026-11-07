#include "CyMediaTestCommon.h"

#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QRunnable>
#include <QThreadPool>

#include <cmath>
#include <algorithm>
#include <functional>
#include <random>
#include <thread>

namespace {
    inline QString tr(const char* s) {
        return QCoreApplication::translate("CyMediaTest", s);
    }

    static const double kPi = 3.14159265358979323846;
    static const double kTau = 6.28318530718;

    // ====================== 并行方案（参考 Calculate） ======================
    //
    // Calculate 里的做法是：线程数取 std::thread::hardware_concurrency()，
    // 每块至少 1K 像素（chunkSize = max(total / (threads * 4), 1024)），
    // 再把分块交给 parallel_for —— 而不是对每一行都单独投递一个并行任务。
    //
    // 原来这里直接 `parallel_for(0, height, ...)`：任务粒度是"一行"，
    // 调度开销远大于计算，而且每帧都会把全部逻辑核拉满。这里改成同样的
    // 分块策略，并额外把生成器的线程数限制为逻辑核数的一半（最多 4 个），
    // 给渲染/采集/解码留出余量，避免测试程序把 CPU 占满。

    /// 生成器线程数：CPU 逻辑核数的一半，至少 1，最多 4。
    inline int generatorThreads() {
        static const int n = []() {
            unsigned int hw = std::thread::hardware_concurrency();
            if (hw == 0) hw = 2;
            const int half = int(std::max(1u, hw / 2u));
            return std::min(half, 4);
        }();
        return n;
    }

    /// 生成器专用线程池（懒创建；线程数固定，避免逐帧创建/销毁线程）。
    /// @note 故意使用进程级生命周期（不释放），避免静态析构顺序与 Qt 退出竞争。
    inline QThreadPool* generatorPool() {
        static QThreadPool* pool = []() {
            QThreadPool* p = new QThreadPool();
            p->setMaxThreadCount(std::max(1, generatorThreads()));
            p->setExpiryTimeout(30000);
            return p;
        }();
        return pool;
    }

    /// 一行块任务：按值捕获行区间，图像缓冲由调用方保证在其返回前有效。
    class RowChunkTask : public QRunnable {
    public:
        explicit RowChunkTask(std::function<void()> fn) : m_fn(std::move(fn)) {
            setAutoDelete(true);
        }
        void run() override {
            if (m_fn) m_fn();
        }

    private:
        std::function<void()> m_fn;
    };

    /**
     * @brief 按 CPU 线程数分块并行处理每一行。
     * @param width    图像宽度（用于把"每块至少 1K 像素"换算成行数）
     * @param height   图像高度
     * @param rowBody  处理单行的回调：rowBody(h)（0 <= h < height）
     * @note 不同块的行区间互不重叠，回调只需保证同一行内的写入是本地的。
     */
    template <typename RowBody>
    void parallelRows(int width, int height, RowBody&& rowBody) {
        constexpr size_t kMinPixelsPerChunk = 1024;  // 与 Calculate 一致：每块至少 1K 像素
        constexpr size_t kSerialPixels = size_t(1) << 16; // 低于该像素数直接串行

        if (height <= 0) return;

        const size_t total = size_t(std::max(1, width)) * size_t(height);
        const int threads = generatorThreads();
        if (threads <= 1 || total < kSerialPixels) {
            for (int h = 0; h < height; ++h) rowBody(h);
            return;
        }

        const int minRows = std::max(1, int(kMinPixelsPerChunk / size_t(std::max(1, width))));
        const int chunkRows = std::max(minRows, height / (threads * 4));
        const int chunkCount = (height + chunkRows - 1) / chunkRows;
        if (chunkCount <= 1) {
            for (int h = 0; h < height; ++h) rowBody(h);
            return;
        }

        QThreadPool* pool = generatorPool();
        for (int c = 0; c < chunkCount; ++c) {
            const int begin = c * chunkRows;
            const int end = std::min(begin + chunkRows, height);
            pool->start(new RowChunkTask([&rowBody, begin, end]() {
                for (int h = begin; h < end; ++h) rowBody(h);
                }));
        }
        // 调用线程等待本帧的所有分块完成（线程池是生成器专用的，不会牵连其它任务）
        pool->waitForDone();
    }

    // Inigo Quilez 余弦调色板：色相连续，非常适合分形平滑着色
    inline void iqPalette(float t, uint8_t& r, uint8_t& g, uint8_t& b) {
        const float tau = 6.28318530718f;
        r = (uint8_t)(255.0f * (0.5f + 0.5f * cosf(tau * (t + 0.00f))));
        g = (uint8_t)(255.0f * (0.5f + 0.5f * cosf(tau * (t + 0.33f))));
        b = (uint8_t)(255.0f * (0.5f + 0.5f * cosf(tau * (t + 0.67f))));
    }
}

namespace CyMediaTest {
    // ====================== HSL 工具 ======================

    void hsl2rgb(const HSL& c, double& r, double& g, double& b) {
        double h = c.h / 360.0;
        h = h - std::floor(h);                          // 归一化到 [0,1)
        if (c.s <= 1e-9) { r = g = b = c.l; return; }

        auto hue2rgb = [](double p, double q, double x) {
            if (x < 0.0) x += 1.0;
            if (x > 1.0) x -= 1.0;
            if (x < 1.0 / 6.0) return p + (q - p) * 6.0 * x;
            if (x < 1.0 / 2.0) return q;
            if (x < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - x) * 6.0;
            return p;
            };

        const double q = (c.l < 0.5) ? c.l * (1.0 + c.s) : c.l + c.s - c.l * c.s;
        const double p = 2.0 * c.l - q;
        r = hue2rgb(p, q, h + 1.0 / 3.0);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0 / 3.0);
    }

    // 色相短路径插值（核心细节：不能跨过 180° 走长路）
    static double lerpHue(double h0, double h1, double t) {
        double d = h1 - h0;
        if (d > 180.0) d -= 360.0;
        if (d < -180.0) d += 360.0;
        double h = h0 + d * t;
        h = h - 360.0 * std::floor(h / 360.0);          // 归一到 [0,360)
        return h;
    }

    HSL lerpHsl(const HSL& a, const HSL& b, double t) {
        return HSL{
            lerpHue(a.h, b.h, t),
            a.s + (b.s - a.s) * t,
            a.l + (b.l - a.l) * t
        };
    }

    // ====================== Palette ======================

    namespace {

        // 从 frameIdx 算出：(当前索引, 下一索引, blend)
        struct CycleState {
            int    curIdx;
            int    nxtIdx;
            double blend;
        };

        inline CycleState computeState(int frameIdx, int N,
            const PaletteCycleConfig& cfg) {
            const int cycleF = std::max(1, cfg.cycleFrames);
            const int holdF = std::max(0, std::min(cfg.holdFrames, cycleF));
            const int transF = (cfg.transFrames > 0)
                ? cfg.transFrames : (cycleF - holdF);

            // frameIdx 可能为负（安全）
            const int safeIdx = ((frameIdx % cycleF) + cycleF) % cycleF;
            const int cycleIdx = (frameIdx >= 0)
                ? frameIdx / cycleF
                : -((-frameIdx + cycleF - 1) / cycleF);

            const int curIdx = ((cycleIdx % N) + N) % N;

            int    nxtIdx = curIdx;
            double blend = 0.0;

            if (safeIdx >= holdF && transF > 0) {
                const double u = double(safeIdx - holdF) / double(transF);
                blend = std::min(1.0, std::max(0.0, u));
                blend = blend * blend * (3.0 - 2.0 * blend);   // smoothstep

                if (cfg.randomNext) {
                    // 确定性 hash：只依赖 cycleIdx，保证并行安全
                    const uint32_t seed = cfg.seed ? cfg.seed : uint32_t(cycleIdx);
                    const uint32_t hsh = seed * 2654435761u;   // Knuth
                    if (N > 1) {
                        nxtIdx = (curIdx + 1 + int(hsh % uint32_t(N - 1))) % N;
                    }
                }
                else {
                    nxtIdx = (curIdx + 1) % N;
                }
            }
            return { curIdx, nxtIdx, blend };
        }

    } // anonymous

    void Palette::sample(int frameIdx,
        double& cR, double& cG, double& cB,
        double& bgR, double& bgG, double& bgB) const {
        const int N = int(std::min(main.size(), bg.size()));
        if (N == 0) {
            // 空调色板：返回中性灰白，保证不崩
            cR = cG = cB = 1.0;
            bgR = bgG = bgB = 0.0;
            return;
        }

        const CycleState s = computeState(frameIdx, N, cfg);
        const HSL cm = lerpHsl(main[s.curIdx], main[s.nxtIdx], s.blend);
        const HSL cbg = lerpHsl(bg[s.curIdx], bg[s.nxtIdx], s.blend);

        hsl2rgb(cm, cR, cG, cB);
        hsl2rgb(cbg, bgR, bgG, bgB);
    }

    void Palette::sampleMain(int frameIdx,
        double& cR, double& cG, double& cB) const {
        double br, bg, bb;
        sample(frameIdx, cR, cG, cB, br, bg, bb);
    }

    void Palette::sampleBg(int frameIdx,
        double& bgR, double& bgG, double& bgB) const {
        double cr, cg, cb;
        sample(frameIdx, cr, cg, cb, bgR, bgG, bgB);
    }

    double Palette::sampleWithBlend(int frameIdx,
        double& cR, double& cG, double& cB,
        double& bgR, double& bgG, double& bgB) const {
        const int N = int(std::min(main.size(), bg.size()));
        if (N == 0) {
            cR = cG = cB = 1.0;
            bgR = bgG = bgB = 0.0;
            return 0.0;
        }
        const CycleState s = computeState(frameIdx, N, cfg);
        const HSL cm = lerpHsl(main[s.curIdx], main[s.nxtIdx], s.blend);
        const HSL cbg = lerpHsl(bg[s.curIdx], bg[s.nxtIdx], s.blend);
        hsl2rgb(cm, cR, cG, cB);
        hsl2rgb(cbg, bgR, bgG, bgB);
        return s.blend;
    }

    void Palette::sampleHsl(int frameIdx, HSL& mainOut, HSL& bgOut) const {
        const int N = int(std::min(main.size(), bg.size()));
        if (N == 0) {
            mainOut = { 0.0, 0.0, 1.0 };
            bgOut = { 0.0, 0.0, 0.0 };
            return;
        }
        const CycleState s = computeState(frameIdx, N, cfg);
        mainOut = lerpHsl(this->main[s.curIdx], this->main[s.nxtIdx], s.blend);
        bgOut = lerpHsl(this->bg[s.curIdx], this->bg[s.nxtIdx], s.blend);
    }

    // ====================== 色阶映射工具 ======================

    void mapToRamp(double v,
        double fgR, double fgG, double fgB,
        double bgR, double bgG, double bgB,
        uint8_t& outR, uint8_t& outG, uint8_t& outB,
        double whiteMix) {
        v = std::min(1.0, std::max(0.0, v));

        // 前段：[bg, fg] 线性
        double rr = bgR + (fgR - bgR) * v;
        double gg = bgG + (fgG - bgG) * v;
        double bb = bgB + (fgB - bgB) * v;

        // 后段：向白推（保留高光感）
        if (whiteMix > 0.0) {
            const double w = v * v * v * whiteMix;
            rr = rr + (1.0 - rr) * w;
            gg = gg + (1.0 - gg) * w;
            bb = bb + (1.0 - bb) * w;
        }

        outR = uint8_t(std::min(255.0, std::max(0.0, rr * 255.0)));
        outG = uint8_t(std::min(255.0, std::max(0.0, gg * 255.0)));
        outB = uint8_t(std::min(255.0, std::max(0.0, bb * 255.0)));
    }

    // ====================== 预置配色 ======================

    namespace {

        Palette makeDeepSea() {
            Palette p;
            p.main = {
                { 195.0, 0.80, 0.62 },   // 深海青蓝
                { 150.0, 0.70, 0.55 },   // 翡翠绿
                {  38.0, 0.85, 0.58 },   // 琥珀金
                {  12.0, 0.90, 0.55 },   // 熔岩红
                { 278.0, 0.75, 0.62 },   // 紫罗兰
            };
            p.bg = {
                { 230.0, 0.70, 0.075 },
                { 170.0, 0.60, 0.065 },
                {  25.0, 0.70, 0.065 },
                {   8.0, 0.75, 0.065 },
                { 270.0, 0.65, 0.075 },
            };
            p.cfg.cycleFrames = 240;
            p.cfg.holdFrames = 170;
            p.cfg.transFrames = 70;
            return p;
        }

        Palette makeAmber() {
            Palette p;
            p.main = {
                {  38.0, 0.85, 0.60 },   // 琥珀
                {  22.0, 0.90, 0.55 },   // 橙
                {  10.0, 0.92, 0.52 },   // 熔岩
            };
            p.bg = {
                {  25.0, 0.75, 0.038 },
                {  15.0, 0.80, 0.038 },
                {   5.0, 0.82, 0.038 },
            };
            p.cfg.cycleFrames = 300;
            p.cfg.holdFrames = 220;
            p.cfg.transFrames = 80;
            return p;
        }

    } // anonymous

    const Palette& paletteDeepSea() {
        static const Palette p = makeDeepSea();
        return p;
    }

    const Palette& paletteAmber() {
        static const Palette p = makeAmber();
        return p;
    }

    const Palette& paletteMono(double hue, double sat) {
        // 注意：paletteMono 用到的 hue/sat 作 key 缓存在 map 里
        static std::map<std::pair<int, int>, Palette> cache;
        const int k1 = int(std::round(hue * 10.0));
        const int k2 = int(std::round(sat * 1000.0));
        auto key = std::make_pair(k1, k2);
        auto it = cache.find(key);
        if (it != cache.end()) return it->second;

        Palette p;
        p.main = { { hue, sat, 0.60 } };
        p.bg = { { hue, sat * 0.8, 0.04 } };
        // 只有一个元素 → 永远停留，不轮播
        p.cfg.cycleFrames = 240;
        p.cfg.holdFrames = 240;
        p.cfg.transFrames = 0;
        return cache.emplace(key, std::move(p)).first->second;
    }

    QString getAnalogImageTypeName(eAnalogImageType type) {
        switch (type) {
        case AnalogImage_RandomColor:   return tr("Random Color");
        case AnalogImage_CheckerBoard:  return tr("Checker Board");
        case AnalogImage_MovingStripes: return tr("Moving Stripes");
        case AnalogImage_Plasma:        return tr("Plasma");
        case AnalogImage_Julia:         return tr("Julia Set");
        case AnalogImage_FractalNoise:  return tr("Fractal Noise (fBm)");

        case AnalogImage_Kaleidoscope:  return tr("Kaleidoscope");
        case AnalogImage_Twirl:         return tr("Twirl");
        case AnalogImage_Tunnel:        return tr("Tunnel");
        case AnalogImage_Cellular:      return tr("Cellular (Worley)");
        case AnalogImage_RadialRipple:  return tr("Radial Ripple");
        case AnalogImage_Spiral:        return tr("Spiral");
        case AnalogImage_Moire:         return tr("Moire");
        }
        return QString("");
    }

    // ----------------------------------------------------------------------------
    // RGB888 -> NV12 (YUV420SP, U 在前)
    //   out[0 .. w*h-1]     : Y 平面
    //   out[w*h .. w*h*3/2-1]: UV 交错平面 (U0 V0 U1 V1 ...)
    // 使用 BT.601 studio swing（Y:16~235, UV:16~240）系数
    // 要求 width/height 为偶数（如为奇数，内部向下取偶，等价于裁剪）
    // ----------------------------------------------------------------------------
    void rgbToNV12(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out) {
        if (!rgb || width <= 0 || height <= 0) { out.clear(); return; }

        const int w = width & ~1;   // 向下取偶
        const int h = height & ~1;

        out.resize(w * h * 3 / 2);
        uint8_t* yPlane = out.data();
        uint8_t* uvPlane = out.data() + w * h;

        // ---------- Y 平面 ----------
        for (int j = 0; j < h; ++j) {
            const uint8_t* srcRow = rgb + static_cast<size_t>(j) * width * 3;
            uint8_t* yRow = yPlane + static_cast<size_t>(j) * w;
            for (int i = 0; i < w; ++i) {
                int R = srcRow[i * 3 + 0];
                int G = srcRow[i * 3 + 1];
                int B = srcRow[i * 3 + 2];
                int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                yRow[i] = static_cast<uint8_t>(Y < 0 ? 0 : (Y > 255 ? 255 : Y));
            }
        }

        // ---------- UV 平面（4:2:0，2x2 求平均） ----------
        int uvIdx = 0;
        for (int j = 0; j < h; j += 2) {
            const uint8_t* row0 = rgb + static_cast<size_t>(j) * width * 3;
            const uint8_t* row1 = rgb + static_cast<size_t>(j + 1) * width * 3;
            for (int i = 0; i < w; i += 2) {
                int R = row0[i * 3 + 0] + row0[(i + 1) * 3 + 0]
                    + row1[i * 3 + 0] + row1[(i + 1) * 3 + 0];
                int G = row0[i * 3 + 1] + row0[(i + 1) * 3 + 1]
                    + row1[i * 3 + 1] + row1[(i + 1) * 3 + 1];
                int B = row0[i * 3 + 2] + row0[(i + 1) * 3 + 2]
                    + row1[i * 3 + 2] + row1[(i + 1) * 3 + 2];
                R >>= 2; G >>= 2; B >>= 2;

                int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                uvPlane[uvIdx++] = static_cast<uint8_t>(U < 0 ? 0 : (U > 255 ? 255 : U));
                uvPlane[uvIdx++] = static_cast<uint8_t>(V < 0 ? 0 : (V > 255 ? 255 : V));
            }
        }
    }
    // ----------------------------------------------------------------------------
    // RGB888 -> NV21 (YUV420SP, V 在前)，与 NV12 仅 UV 排列不同
    // ----------------------------------------------------------------------------
    void rgbToNV21(const uint8_t* rgb, int width, int height, std::vector<uint8_t>& out) {
        if (!rgb || width <= 0 || height <= 0) { out.clear(); return; }

        const int w = width & ~1;
        const int h = height & ~1;

        out.resize(w * h * 3 / 2);
        uint8_t* yPlane = out.data();
        uint8_t* uvPlane = out.data() + w * h;

        // Y 平面（与 NV12 相同）
        for (int j = 0; j < h; ++j) {
            const uint8_t* srcRow = rgb + static_cast<size_t>(j) * width * 3;
            uint8_t* yRow = yPlane + static_cast<size_t>(j) * w;
            for (int i = 0; i < w; ++i) {
                int R = srcRow[i * 3 + 0];
                int G = srcRow[i * 3 + 1];
                int B = srcRow[i * 3 + 2];
                int Y = ((66 * R + 129 * G + 25 * B + 128) >> 8) + 16;
                yRow[i] = static_cast<uint8_t>(Y < 0 ? 0 : (Y > 255 ? 255 : Y));
            }
        }

        // VU 平面（交换写入顺序）
        int uvIdx = 0;
        for (int j = 0; j < h; j += 2) {
            const uint8_t* row0 = rgb + static_cast<size_t>(j) * width * 3;
            const uint8_t* row1 = rgb + static_cast<size_t>(j + 1) * width * 3;
            for (int i = 0; i < w; i += 2) {
                int R = row0[i * 3 + 0] + row0[(i + 1) * 3 + 0]
                    + row1[i * 3 + 0] + row1[(i + 1) * 3 + 0];
                int G = row0[i * 3 + 1] + row0[(i + 1) * 3 + 1]
                    + row1[i * 3 + 1] + row1[(i + 1) * 3 + 1];
                int B = row0[i * 3 + 2] + row0[(i + 1) * 3 + 2]
                    + row1[i * 3 + 2] + row1[(i + 1) * 3 + 2];
                R >>= 2; G >>= 2; B >>= 2;

                int U = ((-38 * R - 74 * G + 112 * B + 128) >> 8) + 128;
                int V = ((112 * R - 94 * G - 18 * B + 128) >> 8) + 128;
                // NV21: V 先 U 后
                uvPlane[uvIdx++] = static_cast<uint8_t>(V < 0 ? 0 : (V > 255 ? 255 : V));
                uvPlane[uvIdx++] = static_cast<uint8_t>(U < 0 ? 0 : (U > 255 ? 255 : U));
            }
        }
    }

    void genAnalogImage(eAnalogImageType type, QImage& img, int frameIdx) {
        uint8_t* pData = img.bits();
        int width = img.width();
        int height = img.height();

        // 所有 pattern 共享的配色方案（想换风格只改这一行）
        const auto& pal = CyMediaTest::paletteDeepSea();

        switch (type) {

        // ---------- 随机彩色（测试用途，保留原样） ----------
        case CyMediaTest::AnalogImage_RandomColor: {
            static std::random_device rd;
            static std::default_random_engine eng(rd());
            std::uniform_int_distribution<int> distr(0, 255);
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    int idx = (h * width + w) * 3;
                    pData[idx + 0] = distr(eng);
                    pData[idx + 1] = distr(eng);
                    pData[idx + 2] = distr(eng);
                }
            }
        } break;

        // ---------- 移动棋盘格（测试用途，保留黑白） ----------
        case CyMediaTest::AnalogImage_CheckerBoard: {
            const int tileSize = 40;
            const int period = tileSize * 2;
            int offsetX = (frameIdx + tileSize) % period;
            int offsetY = (frameIdx / 2) % period;
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    bool white = (((w + offsetX) / tileSize) +
                        ((h + offsetY) / tileSize)) % 2 == 0;
                    uint8_t val = white ? 255 : 0;
                    int idx = (h * width + w) * 3;
                    pData[idx + 0] = val;
                    pData[idx + 1] = val;
                    pData[idx + 2] = val;
                }
            }
        } break;

        // ---------- 动态正弦条纹（原灰度 → 主色渐变） ----------
        case CyMediaTest::AnalogImage_MovingStripes: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            double freq = 0.02;
            double phase = frameIdx * 0.1;
            for (int h = 0; h < height; ++h) {
                for (int w = 0; w < width; ++w) {
                    double val = sin((w + h) * freq + phase) * 0.5 + 0.5;
                    int idx = (h * width + w) * 3;
                    CyMediaTest::mapToRamp(val, cR, cG, cB, bgR, bgG, bgB,
                        pData[idx], pData[idx + 1], pData[idx + 2],
                        /*whiteMix=*/0.3);
                }
            }
        } break;

        // ---------- 等离子分形（固定配色：颜色由 val 驱动，不走轮播） ----------
        case CyMediaTest::AnalogImage_Plasma: {
            const CyMediaTest::HSL baseH{ 195.0, 0.95, 0.70 };
            const CyMediaTest::HSL baseBg{ 210.0, 0.85, 0.10 };

            const double hueSpan = 300.0;   // ★ 180→300，从青蓝一路摆到洋红/橙，跨大半个色环
            const double satScale = 1.0;
            const double minLight = std::max(0.10, baseBg.l);

            const double t = frameIdx * 0.018;

            parallelRows(width, height, [&](int h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double v = (double)h / height;
                for (int w = 0; w < width; ++w) {
                    const double u = (double)w / width;
                    double val =
                        std::sin(u * 10.0 + t) +
                        std::sin(v * 8.0 + t * 1.3) +
                        std::sin((u + v) * 6.0 + t * 0.7) +
                        std::sin(std::sqrt((u - 0.5) * (u - 0.5) +
                            (v - 0.5) * (v - 0.5)) * 15.0 + t);
                    val = (val + 4.0) / 8.0;                         // [0,1]

                    const double phase = val * 2.0 - 1.0;
                    const double hue = baseH.h + phase * (hueSpan * 0.5);
                    const double sat = baseH.s * satScale;

                    const double wobble = std::sin(val * 6.28318) * 0.04;
                    const double light = minLight + (1.0 - minLight)
                        * std::pow(val + wobble, 0.75);

                    double r, g, b;
                    CyMediaTest::hsl2rgb({ hue, sat, light }, r, g, b);
                    row[w * 3 + 0] = (uint8_t)std::min(255.0, std::max(0.0, r * 255.0));
                    row[w * 3 + 1] = (uint8_t)std::min(255.0, std::max(0.0, g * 255.0));
                    row[w * 3 + 2] = (uint8_t)std::min(255.0, std::max(0.0, b * 255.0));
                }
                });
        } break;

         // ---------- Julia 集合（c 在复平面上绕圆运动） ----------
        case CyMediaTest::AnalogImage_Julia: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const int    maxIter = 40;
            const double aspect = (double)width / (double)height;
            const double t = frameIdx * 0.03;
            const double jr = 0.7885 * cos(t);
            const double ji = 0.7885 * sin(t);
            const double scale = 3.0;

            // 逃逸集外部统一用底色（跟随配色）
            const uint8_t outsideR = uint8_t(bgR * 255.0);
            const uint8_t outsideG = uint8_t(bgG * 255.0);
            const uint8_t outsideB = uint8_t(bgB * 255.0);

            parallelRows(width, height, [&](int h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double y0 = (h / (double)height - 0.5) * scale / aspect;
                for (int w = 0; w < width; ++w) {
                    const double x0 = (w / (double)width - 0.5) * scale;
                    double x = x0, y = y0;
                    int iter = 0;
                    while (x * x + y * y <= 4.0 && iter < maxIter) {
                        const double xt = x * x - y * y + jr;
                        y = 2.0 * x * y + ji;
                        x = xt;
                        ++iter;
                    }
                    const int idx = w * 3;
                    if (iter == maxIter) {
                        row[idx] = outsideR;
                        row[idx + 1] = outsideG;
                        row[idx + 2] = outsideB;
                    }
                    else {
                        const double log_zn = log(x * x + y * y) * 0.5;
                        const double nu = log(log_zn / log(2.0)) / log(2.0);
                        const double sn = (iter + 1.0 - nu) / maxIter;
                        CyMediaTest::mapToRamp(sn, cR, cG, cB, bgR, bgG, bgB,
                            row[idx], row[idx + 1], row[idx + 2], 0.7);
                    }
                }
                });
        } break;

        // ---------- fBm 分形噪声 ----------
        case CyMediaTest::AnalogImage_FractalNoise: {
            // 固定基色（不走轮播）
            const CyMediaTest::HSL baseH{ 195.0, 0.85, 0.60 };
            const CyMediaTest::HSL baseBg{ 230.0, 0.70, 0.045 };

            const double hueSpan = 140.0;                       // v 驱动的色相跨度
            const double minLight = std::max(0.03, baseBg.l);

            const double t = frameIdx * 0.02;
            const double aspect = (double)width / (double)height;
            const int    octaves = 6;

            double bgR, bgG, bgB;
            CyMediaTest::hsl2rgb(baseBg, bgR, bgG, bgB);          // 底色也固定

            parallelRows(width, height, [&](int h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    double v = 0.0, amp = 0.5, frq = 1.0;
                    for (int o = 0; o < octaves; ++o) {
                        v += amp * std::sin(xx * frq * 6.28318 + t * (1.0 + o * 0.3))
                            * std::cos(yy * frq * 6.28318 + t * 0.7 * (1.0 + o * 0.5));
                        frq *= 2.0; amp *= 0.5;
                    }
                    v = v * 0.5 + 0.5;                            // [0,1]

                    // ★ 颜色是噪声值的函数，不是时间的函数
                    const double phase = v * 2.0 - 1.0;           // [-1,+1]
                    const double hue = baseH.h + phase * (hueSpan * 0.5);
                    const double light = minLight + (1.0 - minLight) * std::pow(v, 1.3);

                    double cR, cG, cB;
                    CyMediaTest::hsl2rgb({ hue, baseH.s, light }, cR, cG, cB);

                    // 也可保留一点推白高光，让亮区有"闪光"感（推白不随配色）
                    const double whiteMix = v * v * v * 0.35;
                    cR += (1.0 - cR) * whiteMix;
                    cG += (1.0 - cG) * whiteMix;
                    cB += (1.0 - cB) * whiteMix;

                    const int idx = w * 3;
                    row[idx + 0] = (uint8_t)std::min(255.0, std::max(0.0, cR * 255.0));
                    row[idx + 1] = (uint8_t)std::min(255.0, std::max(0.0, cG * 255.0));
                    row[idx + 2] = (uint8_t)std::min(255.0, std::max(0.0, cB * 255.0));
                }
                });
        } break;

        // ---------- 三角筒万花筒（固定配色，不走轮播） ----------
        case CyMediaTest::AnalogImage_Kaleidoscope: {
            const double t = frameIdx * 0.035;
            const double aspect = (double)width / (double)height;

            const CyMediaTest::HSL mainH{ 195.0, 0.85, 0.60 };
            const CyMediaTest::HSL bgH{ 230.0, 0.70, 0.045 };
            double cR, cG, cB; CyMediaTest::hsl2rgb(mainH, cR, cG, cB);
            double bgR, bgG, bgB; CyMediaTest::hsl2rgb(bgH, bgR, bgG, bgB);

            const double hueSpread = 300.0;

            const double segAngle = kPi / 3.0;
            const double halfSeg = segAngle * 0.5;

            struct Bead { double x, y, r, hue; double cr, cg, cb; };
            static const std::vector<Bead> beads = [&]() {
                std::vector<Bead> v;
                std::mt19937 rng(20240521u);
                std::uniform_real_distribution<double> u01(0.0, 1.0);

                const double mid = kPi / 12.0;
                const double midBeads[][2] = {
                    { 0.08, 0.110 }, { 0.25, 0.090 }, { 0.41, 0.070 },
                    { 0.56, 0.055 }, { 0.70, 0.042 }, { 0.84, 0.032 },
                };
                for (auto& rb : midBeads)
                    v.push_back({ std::cos(mid) * rb[0], std::sin(mid) * rb[0], rb[1], u01(rng), 0, 0, 0 });

                const double axisBeads[][2] = {
                    { 0.16, 0.065 }, { 0.33, 0.050 }, { 0.50, 0.038 }, { 0.68, 0.028 },
                };
                for (auto& rb : axisBeads)
                    v.push_back({ rb[0], 0.0, rb[1], u01(rng), 0, 0, 0 });

                for (int i = 0; i < 14; ++i) {
                    const double rr = 0.10 + 0.85 * u01(rng);
                    const double aa = halfSeg * u01(rng);
                    const double r = 0.015 + 0.040 * u01(rng);
                    v.push_back({ std::cos(aa) * rr, std::sin(aa) * rr, r, u01(rng), 0, 0, 0 });
                }
                return v;
                }();

            struct WPoint { double x, y; };
            static const std::vector<WPoint> wpts = [&]() {
                std::vector<WPoint> v;
                std::mt19937 rng(98765u);
                std::uniform_real_distribution<double> u01(0.0, 1.0);
                for (int i = 0; i < 12; ++i) {
                    const double rr = 0.05 + 0.92 * u01(rng);
                    const double aa = halfSeg * u01(rng);
                    v.push_back({ std::cos(aa) * rr, std::sin(aa) * rr });
                }
                return v;
                }();

            std::vector<Bead> dynBeads;
            dynBeads.reserve(beads.size());
            for (const auto& b : beads) {
                Bead d = b;
                d.x += 0.050 * std::sin(t * 0.70 + b.y * 13.0);
                d.y += 0.040 * std::cos(t * 0.90 + b.x * 13.0);
                d.r *= 0.82 + 0.18 * std::sin(t * 1.30 + b.hue * 21.0);
                d.hue = b.hue + 0.10 * std::sin(t * 0.50 + b.x * 8.0);

                const double beadHue = mainH.h + (d.hue - 0.5) * hueSpread;
                const CyMediaTest::HSL bh{ beadHue, mainH.s, mainH.l };
                CyMediaTest::hsl2rgb(bh, d.cr, d.cg, d.cb);

                dynBeads.push_back(d);
            }
            std::vector<WPoint> dynWpts;
            dynWpts.reserve(wpts.size());
            for (const auto& p : wpts) {
                dynWpts.push_back({
                    p.x + 0.040 * std::sin(t * 0.60 + p.y * 18.0),
                    p.y + 0.040 * std::cos(t * 0.80 + p.x * 18.0)
                    });
            }

            const double seamWidth = 0.018;
            const double seamGain = 0.75;
            const double seamSplit = 0.65;
            const double seamSplit2 = 0.40;

            parallelRows(width, height, [&](int h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    const double r = std::sqrt(xx * xx + yy * yy);

                    // ★ 归一化到 [0, 2π)，让 atan2 在 180° 处的 ±π 跳变
                    //   在 fmod(kTau) 之后被完全消除
                    double aNorm = std::atan2(yy, xx) + t;
                    aNorm = std::fmod(aNorm, kTau);
                    if (aNorm < 0) aNorm += kTau;

                    // 扇区内角，折叠到 [0, halfSeg]
                    const int secId = (int)(aNorm / segAngle);
                    double a = aNorm - (double)secId * segAngle;
                    if (a > halfSeg) a = segAngle - a;

                    // 扇区微差：a 的连续偶函数，两端同值 → 任何边界都无阶跃
                    const double u = a / halfSeg;
                    const double s = std::sin(u * kPi);
                    const double secLum = 1.00 + 0.03 * s * s;

                    // 12 条镜线棱边光
                    const double seamDist = std::min(a, halfSeg - a);
                    const bool   isMidSeam = (a > halfSeg * 0.5);
                    const double seamStrength = isMidSeam ? seamSplit2 : seamSplit;
                    const double seamU = seamDist / seamWidth;
                    const double seamGlow = std::exp(-seamU * seamU) * seamGain * seamStrength;

                    const double bx = std::cos(a) * r;
                    const double by = std::sin(a) * r;

                    double d1 = 1e9, d2 = 1e9;
                    for (int k = 0; k < (int)dynWpts.size(); ++k) {
                        const double dx = bx - dynWpts[k].x;
                        const double dy = by - dynWpts[k].y;
                        const double dd = dx * dx + dy * dy;
                        if (dd < d1) { d2 = d1; d1 = dd; }
                        else if (dd < d2) { d2 = dd; }
                    }
                    const double f1 = std::sqrt(d1);
                    const double f2 = std::sqrt(d2);
                    const double edgeF = std::min(1.0, (f2 - f1) * 9.0);

                    const double bgMix = (0.22 + 0.42 * edgeF) * secLum;
                    double rr_ = bgR + (cR - bgR) * bgMix;
                    double gg_ = bgG + (cG - bgG) * bgMix;
                    double bb_ = bgB + (cB - bgB) * bgMix;

                    for (const auto& bead : dynBeads) {
                        const double dx = bx - bead.x;
                        const double dy = by - bead.y;
                        const double d2b = dx * dx + dy * dy;
                        const double rmax = bead.r * 1.10;
                        if (d2b > rmax * rmax) continue;

                        const double d = std::sqrt(d2b);
                        if (d < bead.r) {
                            const double ratio = d / bead.r;
                            const double shell = std::sqrt(std::max(0.0, 1.0 - ratio * ratio));

                            const double ox = bead.r * 0.22 * std::cos(t * 1.3 + bead.y * 18.0);
                            const double oy = bead.r * 0.22 * std::sin(t * 1.3 + bead.x * 18.0);
                            const double idx = bx - bead.x - ox;
                            const double idy = by - bead.y - oy;
                            const double innerR = bead.r * 0.52;
                            const double innerD2 = idx * idx + idy * idy;
                            const double inner = (innerD2 < innerR * innerR)
                                ? (1.0 - std::sqrt(innerD2) / innerR) : 0.0;

                            const double bright = (0.55 + 0.55 * shell + 0.40 * inner)
                                * secLum
                                * (0.85 + 0.15 * bead.hue);
                            rr_ += bead.cr * bright;
                            gg_ += bead.cg * bright;
                            bb_ += bead.cb * bright;
                        }
                        else {
                            const double e = 1.0 - (d - bead.r) / (bead.r * 0.10);
                            const double k = std::max(0.0, e) * 0.6;
                            rr_ += 0.78 * k;
                            gg_ += 0.86 * k;
                            bb_ += 1.00 * k;
                        }
                    }

                    const double core = std::exp(-r * r * 900.0);
                    rr_ += core * 0.94;
                    gg_ += core * 0.82;
                    bb_ += core * 0.63;

                    rr_ += seamGlow * 0.95;
                    gg_ += seamGlow * 0.90;
                    bb_ += seamGlow * 1.00;

                    const double dim = 1.0 - std::min(1.0, r * 1.5);
                    rr_ *= dim; gg_ *= dim; bb_ *= dim;

                    row[w * 3 + 0] = (uint8_t)std::min(255.0, std::max(0.0, rr_ * 255.0));
                    row[w * 3 + 1] = (uint8_t)std::min(255.0, std::max(0.0, gg_ * 255.0));
                    row[w * 3 + 2] = (uint8_t)std::min(255.0, std::max(0.0, bb_ * 255.0));
                }
                });
        } break;

        // ---------- 漩涡扭曲棋盘格（黑白 → 主色两级棋盘） ----------
        case CyMediaTest::AnalogImage_Twirl: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.02;
            const double aspect = (double)width / (double)height;
            const double twirlStrength = 5.0 + std::sin(t) * 2.5;
            const double radius = 0.55;

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    const double r = std::sqrt(xx * xx + yy * yy);
                    double theta = std::atan2(yy, xx);
                    if (r < radius) {
                        const double k = 1.0 - r / radius;
                        theta += k * k * twirlStrength + t;
                    }
                    const double tx = std::cos(theta) * r;
                    const double ty = std::sin(theta) * r;
                    const int cx = (int)std::floor((tx + 0.5) * 20.0);
                    const int cy = (int)std::floor((ty + 0.5) * 20.0);
                    // 原来：黑 / 白 → 现在：暗底色 / 主色
                    const double v = ((cx ^ cy) & 1) ? 0.85 : 0.12;
                    CyMediaTest::mapToRamp(v, cR, cG, cB, bgR, bgG, bgB,
                        row[w * 3], row[w * 3 + 1], row[w * 3 + 2], 0.3);
                }
            }
        } break;

                                           // ---------- 3D 透视隧道（灰 checker → 主色 checker） ----------
        case CyMediaTest::AnalogImage_Tunnel: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.05;
            const double aspect = (double)width / (double)height;

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    double r = std::sqrt(xx * xx + yy * yy);
                    if (r < 1e-4) r = 1e-4;
                    const double a = std::atan2(yy, xx);
                    const double u = 1.0 / r + t * 2.0;
                    const double v = a / kTau;
                    const int cu = (int)std::floor(u * 3.0);
                    const int cv = (int)std::floor(v * 16.0);
                    const double fu = u * 3.0 - cu;
                    const double fv = v * 16.0 - cv;
                    const bool checker = ((cu + cv) & 1) != 0;
                    const bool edge = (fu < 0.08 || fu > 0.92 ||
                        fv < 0.08 || fv > 0.92);
                    double val;
                    if (edge)        val = 0.08;
                    else if (checker) val = 0.85;
                    else              val = 0.35;
                    const double dim = std::min(1.0, r * 2.2);
                    val *= dim;
                    CyMediaTest::mapToRamp(val, cR, cG, cB, bgR, bgG, bgB,
                        row[w * 3], row[w * 3 + 1], row[w * 3 + 2], 0.3);
                }
            }
        } break;

                                            // ---------- Worley 细胞噪声 ----------
        case CyMediaTest::AnalogImage_Cellular: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.03;
            const int    cellCount = 10;
            const double aspect = (double)width / (double)height;

            struct Pt { double x, y; };
            Pt pts[cellCount];
            for (int k = 0; k < cellCount; ++k) {
                const double a = k * 2.39996322972865332;
                const double rr = 0.30 * std::sqrt((k + 0.5) / cellCount) * 2.0;
                pts[k].x = std::cos(a) * rr
                    + std::sin(t * (1.0 + k * 0.4)) * 0.15;
                pts[k].y = std::sin(a) * rr * aspect
                    + std::cos(t * (1.2 + k * 0.3)) * 0.15;
            }

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    double d1 = 1e9, d2 = 1e9;
                    for (int k = 0; k < cellCount; ++k) {
                        const double dx = xx - pts[k].x;
                        const double dy = yy - pts[k].y;
                        const double d = dx * dx + dy * dy;
                        if (d < d1) { d2 = d1; d1 = d; }
                        else if (d < d2) { d2 = d; }
                    }
                    const double sn = std::min(1.0,
                        (std::sqrt(d2) - std::sqrt(d1)) * 3.5);
                    CyMediaTest::mapToRamp(sn, cR, cG, cB, bgR, bgG, bgB,
                        row[w * 3], row[w * 3 + 1], row[w * 3 + 2], 0.6);
                }
            }
        } break;

                                              // ---------- 多中心动态涟漪 ----------
        case CyMediaTest::AnalogImage_RadialRipple: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.08;
            const double aspect = (double)width / (double)height;
            const double src[3][2] = {
                { std::sin(t * 0.4) * 0.25, std::cos(t * 0.5) * 0.25 * aspect },
                { std::cos(t * 0.6) * 0.25, std::sin(t * 0.3) * 0.25 * aspect },
                { std::sin(t * 0.2) * 0.15, std::cos(t * 0.7) * 0.15 * aspect },
            };

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    double acc = 0.0;
                    for (int s = 0; s < 3; ++s) {
                        const double dx = xx - src[s][0];
                        const double dy = yy - src[s][1];
                        const double r = std::sqrt(dx * dx + dy * dy);
                        acc += std::sin(r * 60.0 - t * 8.0 - s * 1.5);
                    }
                    const double v = acc / 6.0 + 0.5;
                    CyMediaTest::mapToRamp(v, cR, cG, cB, bgR, bgG, bgB,
                        row[w * 3], row[w * 3 + 1], row[w * 3 + 2], 0.5);
                }
            }
        } break;

                                                  // ---------- 深海螺旋（保留自定义逻辑：高光推白 + 暗角 + 中心黑洞） ----------
        case CyMediaTest::AnalogImage_Spiral: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.04;
            const double aspect = (double)width / (double)height;

            const double arms = 2.0;
            const double twist = 32.0;

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    const double r = std::sqrt(xx * xx + yy * yy);
                    const double a = std::atan2(yy, xx);

                    const double phase = a * arms + r * twist - t * 4.0;
                    const double c = 0.5 + 0.5 * std::cos(phase);
                    const double c2 = c * c;
                    const double c4 = c2 * c2;
                    const double c8 = c4 * c4;
                    const double c12 = c8 * c4;
                    const double sharp = c12;
                    const double glow = c2 * std::sqrt(c) * 0.32;

                    const double depth = std::exp(-r * 1.8);
                    const double coreMask = 1.0 - std::exp(-r * r * 120.0);
                    const double coreGlow = 0.06 * std::exp(-r * r * 50.0);
                    const double lum = (sharp + glow) * depth * coreMask + coreGlow;

                    // 亮臂推白（白色高光不随配色变化，保持"光"感）
                    const double whiteMix = lum * lum * lum * 0.7;
                    const double cr = cR + (1.0 - cR) * whiteMix;
                    const double cg = cG + (1.0 - cG) * whiteMix;
                    const double cb = cB + (1.0 - cB) * whiteMix;

                    double rr_ = bgR + lum * cr * (1.0 - bgR);
                    double gg_ = bgG + lum * cg * (1.0 - bgG);
                    double bb_ = bgB + lum * cb * (1.0 - bgB);

                    // 暗角
                    const double vig = 1.0 - std::min(1.0, r * 1.2) * 0.3;
                    rr_ *= vig; gg_ *= vig; bb_ *= vig;

                    row[w * 3 + 0] = (uint8_t)std::min(255.0, std::max(0.0, rr_ * 255.0));
                    row[w * 3 + 1] = (uint8_t)std::min(255.0, std::max(0.0, gg_ * 255.0));
                    row[w * 3 + 2] = (uint8_t)std::min(255.0, std::max(0.0, bb_ * 255.0));
                }
            }
        } break;

                                            // ---------- 莫尔干涉条纹 ----------
        case CyMediaTest::AnalogImage_Moire: {
            double cR, cG, cB, bgR, bgG, bgB;
            pal.sample(frameIdx, cR, cG, cB, bgR, bgG, bgB);

            const double t = frameIdx * 0.02;
            const double aspect = (double)width / (double)height;
            const double cx1 = std::sin(t * 0.5) * 0.12;
            const double cy1 = std::cos(t * 0.4) * 0.12 * aspect;
            const double cx2 = -cx1;
            const double cy2 = -cy1;

            for (int h = 0; h < height; ++h) {
                uint8_t* row = pData + (size_t)h * width * 3;
                const double yy = (h / (double)height - 0.5) * aspect;
                for (int w = 0; w < width; ++w) {
                    const double xx = (w / (double)width - 0.5);
                    const double r1 = std::sqrt((xx - cx1) * (xx - cx1) +
                        (yy - cy1) * (yy - cy1));
                    const double r2 = std::sqrt((xx - cx2) * (xx - cx2) +
                        (yy - cy2) * (yy - cy2));
                    const double v = std::sin(r1 * 45.0) * std::sin(r2 * 45.0);
                    const double sn = v * 0.5 + 0.5;
                    CyMediaTest::mapToRamp(sn, cR, cG, cB, bgR, bgG, bgB,
                        row[w * 3], row[w * 3 + 1], row[w * 3 + 2], 0.5);
                }
            }
        } break;

        }
    }
};
