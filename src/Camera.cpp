#include "Camera.h"
#include "CameraParams.h"

#include <cstddef>
#include <iostream>

#include <memory>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>
#include <ostream>

void Camera::Deleter::operator()(void* h)const
{
    if(h)MV_CC_DestroyHandle(h);
}

struct Controller::Impl
{
    MV_CC_DEVICE_INFO_LIST list{};
    bool enumerated = false;
};


static inline bool sdkinitialize()
{
    int nRet = MV_CC_Initialize();
    if(Controller::check(nRet,"sdk intialize"))
    {
        return false;
    }
    return true;
}

static inline bool sdkfinalize()
{
    int nRet = MV_CC_Finalize();
    if(Controller::check(nRet,"sdk finalize"))
    {
        return false;
    }
    return true;
}

Controller::Controller()
{
    sdkinitialize();
    m_impl=std::make_unique<Impl>();
}

Controller::~Controller(){sdkfinalize();};

bool Controller::check(int nRet,std::string msg)
{
    if(nRet != MV_OK)
    {
        std::cerr<<msg<<" error"<<std::hex<<nRet<<std::dec<<std::endl;
        return false;
    }
    return true;
}

Camera::Camera()=default;

Camera::~Camera()
{
    close();
}

bool Controller::enum_device()const
{
    if(m_impl->enumerated)return true;

    memset(&m_impl->list,0,sizeof(m_impl->list));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE|MV_USB_DEVICE, &m_impl->list);
    if(!check(nRet,"enum device"))return 0;
    
    if (m_impl->list.nDeviceNum > 0)
    {
        for (int i = 0; i < m_impl->list.nDeviceNum; i++)
        {
            std::cout<<"device "<<i<<std::endl;
            MV_CC_DEVICE_INFO* pDeviceInfo = m_impl->list.pDeviceInfo[i];
            if (NULL == pDeviceInfo)
            {
                break;
            } 
            std::cout<<"DeviceTypeInfo:"<<pDeviceInfo->nTLayerType<<std::endl;          
        }  
    } 
    else
    {
        std::cout<<"Find No Devices!\n"<<std::endl;
        return 0;
    }
    m_impl->enumerated=true;
    return 1;
}

unsigned Controller::device_num()const
{
    return m_impl->list.nDeviceNum;
}

void* Controller::device_info(unsigned index)const
{
    if(index>=m_impl->list.nDeviceNum || m_impl->list.pDeviceInfo[index] == nullptr)
    {
        return nullptr;
    }
    return static_cast<void*>(m_impl->list.pDeviceInfo[index]);
}

bool Camera::open(Controller& ctrl,unsigned int index)
{
    unsigned num =ctrl.device_num();
    if(index >= num)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }

    int nRet = MV_CC_OpenDevice(m_handle.get());
    if(!Controller::check(nRet,"open"))
    {
        m_handle.reset();
        return false;
    }

    m_open=true;

    return true;
}

bool Camera::get_open()const
{
    bool open = m_open;
    return open;
}

bool Camera::get_grabbing()const
{
    bool grabbing = m_grabbing;
    return grabbing;
}

bool Camera::handle(Controller& ctrl,unsigned int index)
{
    unsigned num = ctrl.device_num();
    const void* info = ctrl.device_info(index);
    if(index >= num)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }
    
    auto* sdkInfo = static_cast<const MV_CC_DEVICE_INFO*>(info);

    void* raw=nullptr;
    int nRet = MV_CC_CreateHandle(&raw, sdkInfo);
    if(!Controller::check(nRet,"creat handle"))return false;

    m_handle.reset(raw);
    if(m_handle.get()==nullptr){
        std::cerr<<"gethandle false"<<std::endl;
        return false;
    }
    return true;
}

void Camera::set_exposure_time(double exposure_time)
{
    if(m_open == false || m_grabbing == true)
    {
        std::cout<<"device not open or grabbing"<<std::endl;
        return;
    }

    const char* strNodeName = "ExposureTime";

    MVCC_FLOATVALUE exposure_time_value;
    MV_CC_GetFloatValue(m_handle.get(), strNodeName , &exposure_time_value);

    int nRet = MV_CC_SetFloatValue(m_handle.get(), strNodeName, exposure_time);
    Controller::check(nRet,"set exposure time");
}

void Camera::set_gain(double gain)
{

    if(m_open == false || m_grabbing == true)
    {
        std::cout<<"device not open or grabbing"<<std::endl;
        return;
    }

    const char* strNodeName = "Gain";

    MVCC_FLOATVALUE gain_value;
    MV_CC_GetFloatValue(m_handle.get(), strNodeName , &gain_value);

    int nRet = MV_CC_SetFloatValue(m_handle.get(), strNodeName, gain);
    Controller::check(nRet,"set gain");
}

bool Camera::start_grabbing()
{
    if(m_open == false)
    {
        std::cout<<"device not open"<<std::endl;
        return false;
    }
    if (m_grabbing == true) 
    {
        std::cout << "already grabbing" << std::endl;
        return true;
    }

    MV_CC_SetEnumValue(m_handle.get(), "TriggerMode", 0); 

    MV_CC_SetImageNodeNum(m_handle.get(), 5);

    int nRet = MV_CC_StartGrabbing(m_handle.get());
    if (!Controller::check(nRet, "StartGrabbing"))
    {
        m_handle.reset();
        return false;
    }

    m_grabbing = true;
    return true;
}

bool Camera::stop_grabbing()
{
    if(m_open == false)
    {
        std::cout<<"device not open"<<std::endl;
        return false;
    }

    if(m_grabbing == false)
    {
        std::cout<<"not grabbing"<<std::endl;
        return false;
    }

    int nRet = MV_CC_StopGrabbing(m_handle.get());
    if (!Controller::check(nRet, "StopGrabbing"))
    {
        return false;
    }

    m_grabbing = false;
    return true;
}

cv::Mat Camera::get_frame(int timeout)
{
    cv::Mat frame;
    if(m_grabbing == false)
    {
        std::cout<<"not grabbing"<<std::endl;
        frame.empty();
    }

    MV_FRAME_OUT stImageInfo;
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT));

    int nRet = MV_CC_GetImageBuffer(m_handle.get(), &stImageInfo, timeout);
    if (nRet != MV_OK) 
    {
        if (nRet != MV_E_NODATA && nRet != MV_E_NOENOUGH_BUF) 
        {
            Controller::check(nRet, "GetImageBuffer");
        }
        frame.empty();
    }

    cv::Mat rawMat(stImageInfo.stFrameInfo.nHeight, 
                   stImageInfo.stFrameInfo.nWidth, 
                   CV_8UC1, 
                   stImageInfo.pBufAddr,
                   stImageInfo.stFrameInfo.nWidth);
                   
    cv::cvtColor(rawMat, frame, cv::COLOR_BayerBG2BGR);

    MV_CC_FreeImageBuffer(m_handle.get(), &stImageInfo);

    return frame;
}

bool Camera::close()
{
    if (m_open == false && m_handle == nullptr)return true;

    int nRet= MV_OK;

    if(m_grabbing == true)
    {
        stop_grabbing();
    }

    if(m_open != false|| m_handle != nullptr)
    {
        nRet = MV_CC_CloseDevice(m_handle.get());
        Controller::check(nRet,"close");
        m_open=false;
    }
    if(m_handle != nullptr)
    {
        m_handle.reset();
    }

    std::cout << "Camera closed" << std::endl;

    return true;
}
