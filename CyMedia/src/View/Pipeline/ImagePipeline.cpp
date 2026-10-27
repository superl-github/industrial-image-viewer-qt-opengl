#include "ImagePipeline.h"

#include "ImageProcessor.h"
#include "common/CyMediaLog.h"
#include "Calculate/Calculate.h"

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QWaitCondition>

#include <atomic>
#include <cstring>
#include <memory>

namespace CyMedia {
    namespace {
        uint8_t* allocAligned(uint32_t bytes) {
#ifdef _MSC_VER
            return static_cast<uint8_t*>(_aligned_malloc(bytes, 64));
#else
            return new uint8_t[bytes];
#endif
        }

        void freeAligned(uint8_t* p) {
            if (!p) return;
#ifdef _MSC_VER
            _aligned_free(p);
#else
            delete[] p;
#endif
        }
    }

    class ImagePipeline::Impl {
    public:
        struct Slot {
            ImageShowInfo info;
            uint8_t* data = nullptr;
            uint32_t capacity = 0;
            bool isSource = false;
            bool copySource = true;
            bool updateDisplay = true;
            bool updateStretch = true;
            bool countFps = true;
            std::atomic<bool> ready{ false };

            Slot() = default;
            Slot(const Slot&) = delete;
            Slot& operator=(const Slot&) = delete;
            ~Slot() { freeAligned(data); }

            void ensureCapacity(uint32_t need) {
                if (capacity >= need) return;
                freeAligned(data);
                data = allocAligned(need);
                capacity = data ? need : 0;
            }
        };

        /// 工作线程取出的不可变任务快照。
        struct WorkItem {
            ImageShowInfo info;
            uint8_t* data = nullptr;
            bool isSource = false;
            bool copySource = true;
            bool updateDisplay = true;
            bool updateStretch = true;
            bool countFps = true;
        };

    public:
        explicit Impl(Sink* s) : sink(s) {
            allocateSlots(kDefaultStack);
            fpsTimer.start();
        }

        ~Impl() { stop(); }

        bool submit(const ImageShowInfo& info, void* data, bool force) {
            if (!data || info.width <= 0 || info.height <= 0 || info.bit <= 0 || info.length <= 0) {
                CyMediaLog::instance().log_printf(LogLevel::ERR,
                    "[ImagePipeline] invalid frame: w=%d h=%d bit=%d len=%u",
                    info.width, info.height, int(info.bit), info.length);
                return false;
            }
            static const int32_t kMaxImageSize = 15000;
            if (info.width > kMaxImageSize || info.height > kMaxImageSize) {
                CyMediaLog::instance().log_printf(LogLevel::WAR,
                    "[ImagePipeline] frame too large: (%d %d), max %d",
                    info.width, info.height, kMaxImageSize);
                return false;
            }

            const Options opt = options();
            if (directMode.load(std::memory_order_acquire)) {
                WorkItem item;
                item.info = info;
                item.data = static_cast<uint8_t*>(data);
                item.isSource = false;
                item.copySource = dataSlow.load() || (++copyCounter >= 10);
                if (copyCounter >= 10) copyCounter = 0;
                item.updateDisplay = true;
                item.updateStretch = true;
                item.countFps = true;
                process(item, directProcessor, opt);
                return true;
            }

            {
                QMutexLocker lock(&mutex);
                Slot* slot = acquireSlotLocked(force);
                if (!slot) return false;

                // 深拷贝入队，upImageData 返回后调用方即可复用 data。
                slot->info = info;
                slot->ensureCapacity(info.length);
                if (!slot->data) {
                    CyMediaLog::instance().log_printf(LogLevel::ERR,
                        "[ImagePipeline] allocate %u bytes failed", info.length);
                    slot->ready.store(false, std::memory_order_release);
                    return false;
                }
                std::memcpy(slot->data, data, info.length);
                slot->isSource = false;
                slot->copySource = dataSlow.load(std::memory_order_relaxed) || (++copyCounter >= 10);
                if (copyCounter >= 10) copyCounter = 0;
                slot->updateDisplay = true;
                slot->updateStretch = true;
                slot->countFps = true;
                slot->ready.store(true, std::memory_order_release);
            }

            ensureThread();
            cond.wakeOne();
            return true;
        }

        void requestReprocess(bool updateDisplay, bool updateStretch, bool force) {
            if (directMode.load(std::memory_order_acquire)) {
                if (sink) sink->onNeedImage();
                if (!haveImage.load(std::memory_order_acquire)) return;

                WorkItem item;
                {
                    QMutexLocker lock(&mutex);
                    if (!rawData || rawInfo.length == 0) return;
                    item.info = rawInfo;
                    item.data = rawData.get();
                }
                item.isSource = true;
                item.copySource = false;
                item.updateDisplay = updateDisplay;
                item.updateStretch = updateStretch;
                item.countFps = false;
                process(item, directProcessor, options());
                return;
            }

            if (!haveImage.load(std::memory_order_acquire)) return;

            {
                QMutexLocker lock(&mutex);
                Slot* slot = acquireSlotLocked(force);
                if (!slot) return;
                slot->isSource = true;
                slot->copySource = false;
                slot->updateDisplay = updateDisplay;
                slot->updateStretch = updateStretch;
                slot->countFps = false;
                slot->ready.store(true, std::memory_order_release);
            }

            ensureThread();
            cond.wakeOne();
        }

        void clear() {
            stopThread();
            {
                QMutexLocker lock(&mutex);
                for (uint32_t i = 0; i < stackSize; ++i) {
                    frameSlots[i].ready.store(false, std::memory_order_release);
                }
                currentSlot.store(-1, std::memory_order_release);
                writeSlot.store(0, std::memory_order_relaxed);
                rawData.reset();
                rawInfo = ImageShowInfo();
                imageInfo = ImageShowInfo();
            }
            haveImage.store(false, std::memory_order_release);
            fpsCount = 0;
            fpsTimer.restart();
            cond.wakeAll();
        }

        void stop() {
            stopThread();
        }

        void setStackSize(uint32_t num) {
            num = num < 1 ? 1 : (num > 10 ? 10 : num);
            if (num == stackSize) return;

            const bool wasRunning = thread && thread->isRunning();
            stopThread();
            {
                QMutexLocker lock(&mutex);
                allocateSlots(num);
            }
            if (wasRunning) {
                ensureThread();
            }
        }

        void setDirect(bool direct) {
            if (directMode.load(std::memory_order_acquire) == direct) return;

            stopThread();
            directMode.store(direct, std::memory_order_release);
            {
                QMutexLocker lock(&mutex);
                for (uint32_t i = 0; i < stackSize; ++i) {
                    frameSlots[i].ready.store(false, std::memory_order_release);
                }
                currentSlot.store(-1, std::memory_order_release);
                writeSlot.store(0, std::memory_order_relaxed);
            }
            if (direct) {
                fpsTimer.restart();
                fpsCount = 0;
            }
        }

        void setOptions(const Options& opt) {
            QMutexLocker lock(&mutex);
            optionsValue = opt;
        }

        Options options() const {
            QMutexLocker lock(&mutex);
            return optionsValue;
        }

        void setImageCallback(ImageFrameCallback cb, void* user) {
            QMutexLocker lock(&mutex);
            imageCallback = std::move(cb);
            imageCallbackUser = user;
        }

        bool samplePixel(int x, int y, double* r, double* g, double* b, bool* gray) {
            QMutexLocker lock(&mutex);
            if (!rawData || rawInfo.length == 0) return false;
            if (x < 0 || y < 0 || x >= rawInfo.width || y >= rawInfo.height) return false;

            ImageShowInfo info = rawInfo;
            ImageColorOpe ope;
            ope.bayerFunc = optionsValue.demosaic;
            ope.YUVFunc = optionsValue.yuv;
            ope.stretchType = optionsValue.stretch;
            ope.stretch_S = optionsValue.stretchRange.start;
            ope.stretch_E = optionsValue.stretchRange.end;

            double rr = 0.0, gg = 0.0, bb = 0.0;
            CyMediaCalc::calcCoordinateColor(info, rawData.get(), x, y, &rr, &gg, &bb, ope);
            if (r) *r = rr;
            if (g) *g = gg;
            if (b) *b = bb;
            if (gray) {
                *gray = info.isMono()
                    || (info.isBayer() && ope.bayerFunc == DEMOSAIC_NONE)
                    || (info.isYUV() && ope.YUVFunc == YUVTRANS_Y);
            }
            return true;
        }

        //==================== 线程 ====================
        void ensureThread() {
            if (!thread) {
                thread = QThread::create([this]() { run(); });
                running.store(true, std::memory_order_release);
                thread->start();
                return;
            }
            if (!thread->isRunning()) {
                running.store(true, std::memory_order_release);
                thread->start();
            }
        }

        void stopThread() {
            running.store(false, std::memory_order_release);
            cond.wakeAll();
            if (thread) {
                thread->quit();
                thread->wait();
                delete thread;
                thread = nullptr;
            }
        }

        void run() {
            while (running.load(std::memory_order_acquire)) {
                // 等待渲染端（OpenGL）就绪。
                while (running.load(std::memory_order_acquire) && sink && !sink->isReady()) {
                    QThread::msleep(1);
                }
                if (!running.load(std::memory_order_acquire)) break;

                int idx = -1;
                WorkItem item;
                Options opt;
                {
                    QMutexLocker lock(&mutex);
                    while (running.load(std::memory_order_acquire) && idx < 0) {
                        for (uint32_t i = 0; i < stackSize; ++i) {
                            if (frameSlots[i].ready.load(std::memory_order_acquire)) {
                                idx = int(i);
                                currentSlot.store(idx, std::memory_order_release);
                                break;
                            }
                        }
                        if (idx < 0) {
                            cond.wait(&mutex);
                        }
                    }
                    if (idx < 0) break;

                    Slot& slot = frameSlots[idx];
                    item.info = slot.info;
                    item.data = slot.data;
                    item.isSource = slot.isSource;
                    item.copySource = slot.copySource;
                    item.updateDisplay = slot.updateDisplay;
                    item.updateStretch = slot.updateStretch;
                    item.countFps = slot.countFps;
                    opt = optionsValue;
                    if (item.isSource) {
                        if (!rawData || rawInfo.length == 0) {
                            slot.ready.store(false, std::memory_order_release);
                            currentSlot.store(-1, std::memory_order_release);
                            continue;
                        }
                        item.info = rawInfo;
                        item.data = rawData.get();
                    }
                }

                process(item, workerProcessor, opt);

                {
                    QMutexLocker lock(&mutex);
                    frameSlots[idx].ready.store(false, std::memory_order_release);
                    currentSlot.store(-1, std::memory_order_release);
                }
            }
        }

        //==================== 处理 ====================
        void process(const WorkItem& item, ImageProcessor& processor, const Options& opt) {
            if (!item.data) return;
            updateFps(item.countFps);

            ImageShowInfo info = item.info;
            void* data = item.data;

            // 保存一份原始帧，供像素采样/参数变化后的重处理。
            if (!item.isSource && item.copySource) {
                copyRawFrame(info, static_cast<const uint8_t*>(data));
            }

            ImageProcessor::Context ctx;
            ctx.colorOpe.bayerFunc = opt.demosaic;
            ctx.colorOpe.YUVFunc = opt.yuv;
            ctx.colorOpe.stretchType = opt.stretch;
            ctx.colorOpe.stretch_S = opt.stretchRange.start;
            ctx.colorOpe.stretch_E = opt.stretchRange.end;
            ctx.bayerToRgbOnCpu = opt.forceCpuBayer || opt.demosaic == DEMOSAIC_AHD;
            ctx.yuvToRgbOnCpu = opt.forceCpuYuv;

            if (!processor.process(info, data, ctx)) {
                CyMediaLog::instance().log_printf(LogLevel::ERR,
                    "[ImagePipeline] format conversion failed, format=%d bit=%d",
                    int(info.format), int(info.bit));
                return;
            }

            haveImage.store(true, std::memory_order_release);
            {
                QMutexLocker lock(&mutex);
                imageInfo = info;
            }

            if (sink) {
                PipelineFrame frame;
                frame.info = info;
                frame.data = static_cast<uint8_t*>(data);
                frame.colorOpe = ctx.colorOpe;
                frame.isSource = item.isSource;
                frame.updateDisplay = item.updateDisplay && !item.isSource;
                frame.updateStretch = item.updateStretch;
                frame.stretchEnabled = opt.stretchPanelVisible;
                frame.stretch = opt.stretchRange;
                sink->onFrame(frame);
            }

            if (!item.isSource) {
                ImageFrameCallback cb;
                void* user = nullptr;
                {
                    QMutexLocker lock(&mutex);
                    cb = imageCallback;
                    user = imageCallbackUser;
                }
                if (cb) {
                    cb(info, static_cast<uint8_t*>(data), user);
                }
            }
        }

        void copyRawFrame(const ImageShowInfo& info, const uint8_t* data) {
            QMutexLocker lock(&mutex);
            if (!rawData || rawInfo.length != info.length) {
                rawData.reset(new uint8_t[info.length]);
            }
            std::memcpy(rawData.get(), data, info.length);
            rawInfo = info;
        }

        void updateFps(bool count) {
            if (count) ++fpsCount;

            if (!fpsTimer.isValid()) fpsTimer.start();
            const qint64 elapsed = fpsTimer.elapsed();
            if (elapsed <= 0) return;

            const bool direct = directMode.load(std::memory_order_acquire);
            const bool enough = direct ? (elapsed >= 1000) : (fpsCount >= 30 || elapsed >= 3000);
            if (!enough) return;

            const double fps = 1000.0 * double(fpsCount) / double(elapsed);
            dataFps.store(fps, std::memory_order_release);
            dataSlow.store(fps < 2.1, std::memory_order_release);
            fpsCount = 0;
            fpsTimer.restart();
        }

        //==================== 队列 ====================
        /// 仅在持有 mutex 时调用。
        Slot* acquireSlotLocked(bool force) {
            uint32_t slot = writeSlot.load(std::memory_order_relaxed) % stackSize;
            for (uint32_t tries = 0; tries < stackSize; ++tries) {
                if (int(slot) != currentSlot.load(std::memory_order_acquire)
                    && !frameSlots[slot].ready.load(std::memory_order_acquire)) {
                    writeSlot.store((slot + 1) % stackSize, std::memory_order_relaxed);
                    frameSlots[slot].ready.store(false, std::memory_order_release);
                    return &frameSlots[slot];
                }
                slot = (slot + 1) % stackSize;
            }
            if (!force) return nullptr;

            slot = uint32_t((currentSlot.load(std::memory_order_acquire) + 1) % int(stackSize));
            frameSlots[slot].ready.store(false, std::memory_order_release);
            return &frameSlots[slot];
        }

        void allocateSlots(uint32_t num) {
            stackSize = num;
            frameSlots.reset(new Slot[num]);
            currentSlot.store(-1, std::memory_order_release);
            writeSlot.store(0, std::memory_order_relaxed);
        }

    public:
        static const uint32_t kDefaultStack = 3;

        Sink* sink = nullptr;
        mutable QMutex mutex;
        QWaitCondition cond;

        QThread* thread = nullptr;
        std::atomic<bool> running{ false };
        std::atomic<bool> directMode{ false };
        std::atomic<bool> haveImage{ false };
        std::atomic<bool> dataSlow{ true };
        std::atomic<double> dataFps{ 0.0 };
        std::atomic<uint32_t> writeSlot{ 0 };
        std::atomic<int> currentSlot{ -1 };

        std::unique_ptr<Slot[]> frameSlots;
        uint32_t stackSize = kDefaultStack;

        ImageShowInfo imageInfo;
        std::shared_ptr<uint8_t[]> rawData;
        ImageShowInfo rawInfo;

        Options optionsValue;
        ImageFrameCallback imageCallback;
        void* imageCallbackUser = nullptr;

        ImageProcessor workerProcessor;
        ImageProcessor directProcessor;

        QElapsedTimer fpsTimer;
        int64_t fpsCount = 0;
        int copyCounter = 0;
    };

    ImagePipeline::ImagePipeline(Sink* sink)
        : d(new Impl(sink)) {
    }

    ImagePipeline::~ImagePipeline() {
        delete d;
        d = nullptr;
    }

    bool ImagePipeline::submit(const ImageShowInfo& info, void* data, bool force) {
        return d->submit(info, data, force);
    }

    void ImagePipeline::setStackSize(uint32_t num) {
        d->setStackSize(num);
    }

    uint32_t ImagePipeline::stackSize() const {
        return d->stackSize;
    }

    void ImagePipeline::setDirect(bool direct) {
        d->setDirect(direct);
    }

    bool ImagePipeline::direct() const {
        return d->directMode.load(std::memory_order_acquire);
    }

    void ImagePipeline::requestReprocess(bool updateDisplay, bool updateStretch, bool force) {
        d->requestReprocess(updateDisplay, updateStretch, force);
    }

    void ImagePipeline::clear() {
        d->clear();
    }

    bool ImagePipeline::hasImage() const {
        return d->haveImage.load(std::memory_order_acquire);
    }

    ImageShowInfo ImagePipeline::imageInfo() const {
        QMutexLocker lock(&d->mutex);
        return d->imageInfo;
    }

    const ImageShowInfo& ImagePipeline::imageInfoRef() const {
        return d->imageInfo;
    }

    double ImagePipeline::dataFps() const {
        return d->dataFps.load(std::memory_order_acquire);
    }

    bool ImagePipeline::isDataSlow() const {
        return d->dataSlow.load(std::memory_order_acquire);
    }

    bool ImagePipeline::samplePixel(int x, int y, double* r, double* g, double* b, bool* gray) {
        return d->samplePixel(x, y, r, g, b, gray);
    }

    void ImagePipeline::setOptions(const Options& opt) {
        d->setOptions(opt);
    }

    ImagePipeline::Options ImagePipeline::options() const {
        return d->options();
    }

    void ImagePipeline::setImageCallback(ImageFrameCallback cb, void* user) {
        d->setImageCallback(std::move(cb), user);
    }

    void ImagePipeline::stop() {
        d->stop();
    }
}

