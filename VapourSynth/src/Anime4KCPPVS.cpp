#include <VapourSynth.h>
#include <VSHelper.h>

#include "Anime4KCPP.hpp"

enum class GPGPU
{
    OpenCL, CUDA
};

typedef struct Anime4KCPPData {
    VSNodeRef* node = nullptr;
    VSVideoInfo vi = VSVideoInfo();
    int passes = 2;
    int pushColorCount = 2;
    double strengthColor = 0.3;
    double strengthGradient = 1.0;
    double zoomFactor = 2.0;
    bool GPU = false;
    bool CNN = false;
    bool HDN = false;
    int HDNLevel = 1;
    unsigned int pID = 0, dID = 0;
    Anime4KCPP::ACCreator acCreator;
    Anime4KCPP::Parameters parameters;
    GPGPU GPGPUModel = GPGPU::OpenCL;
}Anime4KCPPData;

static void VS_CC Anime4KCPPInit(VSMap* in, VSMap* out, void** instanceData, VSNode* node, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)(*instanceData);

    if (data->GPU)
    {
        switch (data->GPGPUModel)
        {
        case GPGPU::OpenCL:
            if (data->CNN)
                data->acCreator.pushManager<Anime4KCPP::OpenCL::Manager<Anime4KCPP::OpenCL::ACNet>>(data->pID, data->dID);
            else
                data->acCreator.pushManager<Anime4KCPP::OpenCL::Manager<Anime4KCPP::OpenCL::Anime4K09>>(data->pID, data->dID);
            break;
        case GPGPU::CUDA:
#ifdef ENABLE_CUDA
            data->acCreator.pushManager<Anime4KCPP::Cuda::Manager>(data->dID);
#endif // ENABLE_CUDA
            break;
        }
        data->acCreator.init();
    }

    data->parameters.passes = data->passes;
    data->parameters.pushColorCount = data->pushColorCount;
    data->parameters.strengthColor = data->strengthColor;
    data->parameters.strengthGradient = data->strengthGradient;
    data->parameters.zoomFactor = data->zoomFactor;
    data->parameters.HDN = data->HDN;
    data->parameters.HDNLevel = data->HDNLevel;

    vsapi->setVideoInfo(&data->vi, 1, node);
}

static const VSFrameRef* VS_CC Anime4KCPPGetFrame(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)(*instanceData);

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, data->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = vsapi->getFrameFilter(n, data->node, frameCtx);

        int h = vsapi->getFrameHeight(src, 0);

        VSFrameRef* dst = vsapi->newVideoFrame(data->vi.format, data->vi.width, data->vi.height, src, core);

        int srcSrtide = vsapi->getStride(src, 0);

        unsigned char* srcR = const_cast<unsigned char*>(vsapi->getReadPtr(src, 0));
        unsigned char* srcG = const_cast<unsigned char*>(vsapi->getReadPtr(src, 1));
        unsigned char* srcB = const_cast<unsigned char*>(vsapi->getReadPtr(src, 2));

        unsigned char* dstR = vsapi->getWritePtr(dst, 0);
        unsigned char* dstG = vsapi->getWritePtr(dst, 1);
        unsigned char* dstB = vsapi->getWritePtr(dst, 2);

        Anime4KCPP::AC* ac = nullptr;

        if (data->GPU)
        {
            switch (data->GPGPUModel)
            {
            case GPGPU::OpenCL:
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_Anime4K09);
                break;
            case GPGPU::CUDA:
#ifdef ENABLE_CUDA
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_Anime4K09);
#endif
                break;
            }
        }
        else
        {
            if (data->CNN)
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_ACNet);
            else
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_Anime4K09);
        }

        ac->loadImage(h, srcSrtide, srcR, srcG, srcB);
        ac->process();
        ac->saveImage(dstR, dstG, dstB);

        data->acCreator.release(ac);
        vsapi->freeFrame(src);

        return dst;
    }
    return nullptr;
}

static const VSFrameRef* VS_CC Anime4KCPPGetFrameYUV(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)(*instanceData);

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, data->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = vsapi->getFrameFilter(n, data->node, frameCtx);

        int h = vsapi->getFrameHeight(src, 0);

        VSFrameRef* dst = vsapi->newVideoFrame(data->vi.format, data->vi.width, data->vi.height, src, core);

        int srcSrtide = vsapi->getStride(src, 0);

        unsigned char* srcY = const_cast<unsigned char*>(vsapi->getReadPtr(src, 0));
        unsigned char* srcU = const_cast<unsigned char*>(vsapi->getReadPtr(src, 1));
        unsigned char* srcV = const_cast<unsigned char*>(vsapi->getReadPtr(src, 2));

        unsigned char* dstY = vsapi->getWritePtr(dst, 0);
        unsigned char* dstU = vsapi->getWritePtr(dst, 1);
        unsigned char* dstV = vsapi->getWritePtr(dst, 2);

        Anime4KCPP::AC* ac = nullptr;

        if (data->GPU)
        {
            switch (data->GPGPUModel)
            {
            case GPGPU::OpenCL:
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_Anime4K09);
                break;
            case GPGPU::CUDA:
#ifdef ENABLE_CUDA
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_Anime4K09);
#endif
                break;
            }
        }
        else
        {
            if (data->CNN)
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_ACNet);
            else
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_Anime4K09);
        }

        ac->loadImage(h, srcSrtide, srcY, srcU, srcV, true);
        ac->process();
        ac->saveImage(dstY, dstU, dstV);

        data->acCreator.release(ac);
        vsapi->freeFrame(src);

        return dst;
    }
    return nullptr;
}

static const VSFrameRef* VS_CC Anime4KCPPGetFrameSafe(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)(*instanceData);

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, data->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = vsapi->getFrameFilter(n, data->node, frameCtx);

        size_t srcH = vsapi->getFrameHeight(src, 0);
        size_t srcW = vsapi->getFrameWidth(src, 0);

        size_t srcSrtide = vsapi->getStride(src, 0);

        VSFrameRef* dst = vsapi->newVideoFrame(data->vi.format, data->vi.width, data->vi.height, src, core);

        size_t dstH = vsapi->getFrameHeight(dst, 0);
        size_t dstW = vsapi->getFrameWidth(dst, 0);

        size_t dstSrtide = vsapi->getStride(dst, 0);

        const unsigned char* srcR = vsapi->getReadPtr(src, 0);
        const unsigned char* srcG = vsapi->getReadPtr(src, 1);
        const unsigned char* srcB = vsapi->getReadPtr(src, 2);

        size_t srcDataSize = srcH * srcW;
        unsigned char* srcRSafe = new unsigned char[srcDataSize];
        unsigned char* srcGSafe = new unsigned char[srcDataSize];
        unsigned char* srcBSafe = new unsigned char[srcDataSize];

        unsigned char* dstR = vsapi->getWritePtr(dst, 0);
        unsigned char* dstG = vsapi->getWritePtr(dst, 1);
        unsigned char* dstB = vsapi->getWritePtr(dst, 2);

        cv::Mat dstRSafe;
        cv::Mat dstGSafe;
        cv::Mat dstBSafe;

        for (size_t y = 0; y < srcH; y++)
        {
            memcpy(srcRSafe + y * srcW, srcR, srcW);
            memcpy(srcGSafe + y * srcW, srcG, srcW);
            memcpy(srcBSafe + y * srcW, srcB, srcW);
            srcR += srcSrtide;
            srcG += srcSrtide;
            srcB += srcSrtide;
        }

        Anime4KCPP::AC* ac = nullptr;

        if (data->GPU)
        {
            switch (data->GPGPUModel)
            {
            case GPGPU::OpenCL:
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_Anime4K09);
                break;
            case GPGPU::CUDA:
#ifdef ENABLE_CUDA
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_Anime4K09);
#endif
                break;
            }
        }
        else
        {
            if (data->CNN)
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_ACNet);
            else
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_Anime4K09);
        }

        ac->loadImage(srcH, srcW, srcRSafe, srcGSafe, srcBSafe);
        ac->process();
        ac->saveImage(dstRSafe, dstGSafe, dstBSafe);

        for (size_t y = 0; y < dstH; y++)
        {
            memcpy(dstR, dstRSafe.data + y * dstW, dstW);
            memcpy(dstG, dstGSafe.data + y * dstW, dstW);
            memcpy(dstB, dstBSafe.data + y * dstW, dstW);
            dstR += dstSrtide;
            dstG += dstSrtide;
            dstB += dstSrtide;
        }

        data->acCreator.release(ac);

        delete[] srcRSafe;
        delete[] srcGSafe;
        delete[] srcBSafe;

        vsapi->freeFrame(src);

        return dst;
    }
    return nullptr;
}

static const VSFrameRef* VS_CC Anime4KCPPGetFrameYUVSafe(int n, int activationReason, void** instanceData, void** frameData, VSFrameContext* frameCtx, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)(*instanceData);

    if (activationReason == arInitial)
        vsapi->requestFrameFilter(n, data->node, frameCtx);
    else if (activationReason == arAllFramesReady)
    {
        const VSFrameRef* src = vsapi->getFrameFilter(n, data->node, frameCtx);

        size_t srcHY = vsapi->getFrameHeight(src, 0);
        size_t srcWY = vsapi->getFrameWidth(src, 0);
        size_t srcHU = vsapi->getFrameHeight(src, 1);
        size_t srcWU = vsapi->getFrameWidth(src, 1);
        size_t srcHV = vsapi->getFrameHeight(src, 2);
        size_t srcWV = vsapi->getFrameWidth(src, 2);

        size_t srcSrtideY = vsapi->getStride(src, 0);
        size_t srcSrtideU = vsapi->getStride(src, 1);
        size_t srcSrtideV = vsapi->getStride(src, 2);

        VSFrameRef* dst = vsapi->newVideoFrame(data->vi.format, data->vi.width, data->vi.height, src, core);

        size_t dstHY = vsapi->getFrameHeight(dst, 0);
        size_t dstWY = vsapi->getFrameWidth(dst, 0);
        size_t dstHU = vsapi->getFrameHeight(dst, 1);
        size_t dstWU = vsapi->getFrameWidth(dst, 1);
        size_t dstHV = vsapi->getFrameHeight(dst, 2);
        size_t dstWV = vsapi->getFrameWidth(dst, 2);

        size_t dstSrtideY = vsapi->getStride(dst, 0);
        size_t dstSrtideU = vsapi->getStride(dst, 1);
        size_t dstSrtideV = vsapi->getStride(dst, 2);

        const unsigned char* srcY = vsapi->getReadPtr(src, 0);
        const unsigned char* srcU = vsapi->getReadPtr(src, 1);
        const unsigned char* srcV = vsapi->getReadPtr(src, 2);

        unsigned char* srcYSafe = new unsigned char[srcHY * srcWY];
        unsigned char* srcUSafe = new unsigned char[srcHU * srcWU];
        unsigned char* srcVSafe = new unsigned char[srcHV * srcWV];

        unsigned char* dstY = vsapi->getWritePtr(dst, 0);
        unsigned char* dstU = vsapi->getWritePtr(dst, 1);
        unsigned char* dstV = vsapi->getWritePtr(dst, 2);

        cv::Mat dstYSafe;
        cv::Mat dstUSafe;
        cv::Mat dstVSafe;

        for (size_t y = 0; y < srcHY; y++)
        {
            memcpy(srcYSafe + y * srcWY, srcY, srcWY);
            srcY += srcSrtideY;
            if (y < srcHU)
            {
                memcpy(srcUSafe + y * srcWU, srcU, srcWU);
                srcU += srcSrtideU;
            }
            if (y < srcHV)
            {
                memcpy(srcVSafe + y * srcWV, srcV, srcWV);
                srcV += srcSrtideV;
            }
        }

        Anime4KCPP::AC* ac = nullptr;

        if (data->GPU)
        {
            switch (data->GPGPUModel)
            {
            case GPGPU::OpenCL:
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::OpenCL_Anime4K09);
                break;
            case GPGPU::CUDA:
#ifdef ENABLE_CUDA
                if (data->CNN)
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_ACNet);
                else
                    ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::Cuda_Anime4K09);
#endif
                break;
            }
        }
        else
        {
            if (data->CNN)
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_ACNet);
            else
                ac = data->acCreator.create(data->parameters, Anime4KCPP::Processor::Type::CPU_Anime4K09);
        }

        ac->loadImage(srcHY, srcWY, srcYSafe, srcHU, srcWU, srcUSafe, srcHV, srcWV, srcVSafe);
        ac->process();
        ac->saveImage(dstYSafe, dstUSafe, dstVSafe);

        for (size_t y = 0; y < dstHY; y++)
        {
            memcpy(dstY, dstYSafe.data + y * dstWY, dstWY);
            dstY += dstSrtideY;
            if (y < dstHU)
            {
                memcpy(dstU, dstUSafe.data + y * dstWU, dstWU);
                dstU += dstSrtideU;
            }
            if (y < dstHV)
            {
                memcpy(dstV, dstVSafe.data + y * dstWV, dstWV);
                dstV += dstSrtideV;
            }
        }

        data->acCreator.release(ac);

        delete[] srcYSafe;
        delete[] srcUSafe;
        delete[] srcVSafe;

        vsapi->freeFrame(src);

        return dst;
    }
    return nullptr;
}

static void VS_CC Anime4KCPPFree(void* instanceData, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData* data = (Anime4KCPPData*)instanceData;
    vsapi->freeNode(data->node);
    delete data;
}

static void VS_CC Anime4KCPPCreate(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi)
{
    Anime4KCPPData tmpData;
    int err = 0;

    tmpData.node = vsapi->propGetNode(in, "src", 0, 0);
    tmpData.vi = *vsapi->getVideoInfo(tmpData.node);

    if (!isConstantFormat(&tmpData.vi) || (tmpData.vi.format->id != pfRGB24 && (tmpData.vi.format->bitsPerSample != 8 || tmpData.vi.format->colorFamily != cmYUV)))
    {
        vsapi->setError(out, "Anime4KCPP: only RGB24 or YUV 8bit supported");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.passes = vsapi->propGetInt(in, "passes", 0, &err);
    if (err)
        tmpData.passes = 2;

    tmpData.pushColorCount = vsapi->propGetInt(in, "pushColorCount", 0, &err);
    if (err)
        tmpData.pushColorCount = 2;

    tmpData.strengthColor = vsapi->propGetFloat(in, "strengthColor", 0, &err);
    if (err)
        tmpData.strengthColor = 0.3;
    else if (tmpData.strengthColor < 0.0 || tmpData.strengthColor > 1.0)
    {
        vsapi->setError(out, "Anime4KCPP: strengthColor must range from 0 to 1");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.strengthGradient = vsapi->propGetFloat(in, "strengthGradient", 0, &err);
    if (err)
        tmpData.strengthGradient = 1.0;
    else if (tmpData.strengthGradient < 0.0 || tmpData.strengthGradient > 1.0)
    {
        vsapi->setError(out, "Anime4KCPP: strengthGradient must range from 0 to 1");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.zoomFactor = vsapi->propGetInt(in, "zoomFactor", 0, &err);
    if (err)
        tmpData.zoomFactor = 1.0;
    else if (tmpData.zoomFactor < 1.0)
    {
        vsapi->setError(out, "Anime4KCPP: zoomFactor must be an integer which >= 1");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.CNN = vsapi->propGetInt(in, "ACNet", 0, &err);
    if (err)
        tmpData.CNN = false;

    if (tmpData.vi.format->id != pfRGB24 && tmpData.vi.format->id != pfYUV444P8 && tmpData.CNN == false)
    {
        vsapi->setError(out, "Anime4KCPP: RGB24 or YUV444P8 only for Anime4K09");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.HDN = vsapi->propGetInt(in, "HDN", 0, &err);
    if (err)
        tmpData.HDN = false;

    tmpData.HDNLevel = vsapi->propGetInt(in, "HDNLevel", 0, &err);
    if (err)
        tmpData.HDNLevel = 1;
    else if (tmpData.HDNLevel < 1 || tmpData.HDNLevel > 3)
    {
        vsapi->setError(out, "Anime4KCPP: HDNLevel must range from 1 to 3");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.GPU = vsapi->propGetInt(in, "GPUMode", 0, &err);
    if (err)
        tmpData.GPU = false;

    std::string GPGPUModel;
    const char* tmpStr = vsapi->propGetData(in, "GPGPUModel", 0, &err);
    if (err)
        GPGPUModel = "opencl";
    else
        GPGPUModel = tmpStr;
    std::transform(GPGPUModel.begin(), GPGPUModel.end(), GPGPUModel.begin(), ::tolower);

    if (GPGPUModel == "opencl")
        tmpData.GPGPUModel = GPGPU::OpenCL;
    else if(GPGPUModel == "cuda")
    {
        
#ifndef ENABLE_CUDA
        vsapi->setError(out, "Anime4KCPP: CUDA is unsupported");
        vsapi->freeNode(tmpData.node);
        return;
#endif // !ENABLE_CUDA
        tmpData.GPGPUModel = GPGPU::CUDA;
    }
    else
    {
        vsapi->setError(out, "Anime4KCPP: GPGPUModel must be \"cuda\" or \"opencl\"");
        vsapi->freeNode(tmpData.node);
        return;
    }

    tmpData.pID = vsapi->propGetInt(in, "platformID", 0, &err);
    if (err || !tmpData.GPU)
        tmpData.pID = 0;

    tmpData.dID = vsapi->propGetInt(in, "deviceID", 0, &err);
    if (err || !tmpData.GPU)
        tmpData.dID = 0;

    bool safeMode = vsapi->propGetInt(in, "safeMode", 0, &err);
    if (err)
        safeMode = true;

    if (!safeMode && tmpData.vi.format->id != pfRGB24 && tmpData.vi.format->id != pfYUV444P8)
    {
        vsapi->setError(out, "Anime4KCPP: RGB24 or YUV444P8 only if safeMode was disabled");
        vsapi->freeNode(tmpData.node);
        return;
    }

    if (tmpData.GPU)
    {
        std::string info;
        try
        {
            switch (tmpData.GPGPUModel)
            {
            case GPGPU::OpenCL:
            {
                Anime4KCPP::OpenCL::GPUList list = Anime4KCPP::OpenCL::listGPUs();
                if (tmpData.pID >= static_cast<unsigned int>(list.platforms) ||
                    tmpData.dID >= static_cast<unsigned int>(list[tmpData.pID]))
                {
                    std::ostringstream err;
                    err << "Platform ID or device ID index out of range" << std::endl
                        << "Run core.anim4kcpp.listGPUs for available platforms and devices" << std::endl
                        << "Your input is: " << std::endl
                        << "    platform ID: " << tmpData.pID << std::endl
                        << "    device ID: " << tmpData.dID << std::endl;
                    vsapi->setError(out, err.str().c_str());
                    vsapi->freeNode(tmpData.node);
                    return;
                }
                Anime4KCPP::OpenCL::GPUInfo ret =
                    Anime4KCPP::OpenCL::checkGPUSupport(tmpData.pID, tmpData.dID);
                if (!ret)
                {
                    std::ostringstream err;
                    err << "The current device is unavailable" << std::endl
                        << "Your input is: " << std::endl
                        << "    platform ID: " << tmpData.pID << std::endl
                        << "    device ID: " << tmpData.dID << std::endl
                        << "Error: " << std::endl
                        << "    " + ret() << std::endl;
                    vsapi->setError(out, err.str().c_str());
                    vsapi->freeNode(tmpData.node);
                    return;
                }
                info = ret();
            }
            break;
            case GPGPU::CUDA:
#ifdef ENABLE_CUDA
            {
                Anime4KCPP::Cuda::GPUList list = Anime4KCPP::Cuda::listGPUs();
                if (list.devices == 0)
                {
                    vsapi->setError(out, list().c_str());
                    vsapi->freeNode(tmpData.node);
                    return;
                }
                else if (tmpData.dID >= list.devices)
                {
                    std::ostringstream err;
                    err << "Device ID index out of range" << std::endl
                        << "Run core.anim4kcpp.listGPUs for available CUDA devices" << std::endl
                        << "Your input is: " << std::endl
                        << "    device ID: " << tmpData.dID << std::endl;
                    vsapi->setError(out, err.str().c_str());
                    vsapi->freeNode(tmpData.node);
                    return;
                }
                Anime4KCPP::Cuda::GPUInfo ret =
                    Anime4KCPP::Cuda::checkGPUSupport(tmpData.dID);
                if (!ret)
                {
                    std::ostringstream err;
                    err << "The current device is unavailable" << std::endl
                        << "Your input is: " << std::endl
                        << "    device ID: " << tmpData.dID << std::endl
                        << "Error: " << std::endl
                        << "    " + ret() << std::endl;
                    vsapi->setError(out, err.str().c_str());
                    vsapi->freeNode(tmpData.node);
                    return;
                }
                info = ret();
            }
            break;
#endif // ENABLE_CUDA
            }
        }
        catch (const std::exception& err)
        {
            vsapi->setError(out, err.what());
            vsapi->freeNode(tmpData.node);
            return;
        }
        vsapi->logMessage(mtDebug, ("Current GPU infomation: \n" + info).c_str());
    }

    if (tmpData.zoomFactor != 1.0)
    {
        if (!safeMode && tmpData.vi.width % 32 != 0)//32-byte alignment
        {
            tmpData.vi.width = ((tmpData.vi.width >> 5) + 1) << 5;
            vsapi->logMessage(mtWarning,
                "The width of the input video is not a multiple of 32 (required by VapourSynth), "
                "there will be black border of output video, please cut it off manually.");
        }
        tmpData.vi.width *= tmpData.zoomFactor;
        tmpData.vi.height *= tmpData.zoomFactor;
    }

    Anime4KCPPData* data = new Anime4KCPPData;
    *data = tmpData;

    if (safeMode)
        if (tmpData.vi.format->colorFamily == cmYUV)
            vsapi->createFilter(in, out, "Anime4KCPP", Anime4KCPPInit, Anime4KCPPGetFrameYUVSafe, Anime4KCPPFree, fmParallel, 0, data, core);
        else
            vsapi->createFilter(in, out, "Anime4KCPP", Anime4KCPPInit, Anime4KCPPGetFrameSafe, Anime4KCPPFree, fmParallel, 0, data, core);
    else
        if (tmpData.vi.format->colorFamily == cmYUV)
            vsapi->createFilter(in, out, "Anime4KCPP", Anime4KCPPInit, Anime4KCPPGetFrameYUV, Anime4KCPPFree, fmParallel, 0, data, core);
        else
            vsapi->createFilter(in, out, "Anime4KCPP", Anime4KCPPInit, Anime4KCPPGetFrame, Anime4KCPPFree, fmParallel, 0, data, core);
}

static void VS_CC Anime4KCPPListGPUs(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi)
{
    int err;
    std::string GPGPUModel;
    const char * tmpStr = vsapi->propGetData(in, "GPGPUModel", 0, &err);
    if (err)
        GPGPUModel = "opencl";
    else
        GPGPUModel = tmpStr;
    std::transform(GPGPUModel.begin(), GPGPUModel.end(), GPGPUModel.begin(), ::tolower);

    if(GPGPUModel=="opencl")
        vsapi->logMessage(mtDebug, Anime4KCPP::OpenCL::listGPUs()().c_str());
    else
#ifdef ENABLE_CUDA
        if (GPGPUModel == "cuda")
            vsapi->logMessage(mtDebug, Anime4KCPP::Cuda::listGPUs()().c_str());
        else
#endif // ENABLE_CUDA
        vsapi->logMessage(mtDebug, "unkonwn GPGPUModel module");
}

static void VS_CC Anime4KCPPBenchmark(const VSMap* in, VSMap* out, void* userData, VSCore* core, const VSAPI* vsapi)
{
    int err = 0;
    int pID = vsapi->propGetInt(in, "platformID", 0, &err);
    if (err || !pID)
        pID = 0;

    int dID = vsapi->propGetInt(in, "deviceID", 0, &err);
    if (err || !dID)
        dID = 0;

    std::pair<double, double> ret = Anime4KCPP::benchmark(pID, dID);
    
    std::ostringstream oss;
    oss << "Benchmark result:" << std::endl
        << "CPU score: " << ret.first << std::endl
        << "GPU score: " << ret.second << std::endl
        << " (pID = " << pID << ", dID = " << dID << ")" << std::endl;

    vsapi->logMessage(mtDebug, oss.str().c_str());
}

VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin* plugin)
{
    configFunc("github.tianzerl.anime4kcpp", "anime4kcpp", "Anime4KCPP for VapourSynth", VAPOURSYNTH_API_VERSION, 1, plugin);
    registerFunc("Anime4KCPP",
        "src:clip;"
        "passes:int:opt;"
        "pushColorCount:int:opt;"
        "strengthColor:float:opt;"
        "strengthGradient:float:opt;"
        "zoomFactor:int:opt;"
        "ACNet:int:opt;"
        "GPUMode:int:opt;"
        "GPGPUModel:data:opt;"
        "HDN:int:opt;"
        "HDNLevel:int:opt;"
        "platformID:int:opt;"
        "deviceID:int:opt;"
        "safeMode:int:opt",
        Anime4KCPPCreate, nullptr, plugin);

    registerFunc("listGPUs", "GPGPUModel:data:opt", Anime4KCPPListGPUs, nullptr, plugin);

    registerFunc("benchmark",
        "platformID:int:opt;"
        "deviceID:int:opt;",
        Anime4KCPPBenchmark, nullptr, plugin);
}
