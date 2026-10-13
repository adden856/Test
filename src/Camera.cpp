#include "Camera.h"

#include <cstddef>
#include <iostream>

#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>

MV_CC_DEVICE_INFO_LIST m_stDeviceList;


bool Controller::check(int nRet,std::string msg)
{
    if(nRet != MV_OK)
    {
        std::cerr<<msg<<" error"<<std::hex<<nRet<<std::dec<<std::endl;
        return false;
    }
    return true;
}

bool SDK::initialize()
{
    int nRet = MV_CC_Initialize();
    if(Controller::check(nRet,"sdk intialize"))
    {
        return false;
    }
    return true;
}

bool SDK::finalize()
{
    int nRet = MV_CC_Finalize();
    if(Controller::check(nRet,"sdk finalize"))
    {
        return false;
    }
    return true;
}

Camera::Camera()
{
    m_handle=nullptr;
    m_open=0;
    m_grabbing=0;
}

Camera::~Camera()
{
    close();
}

bool Controller::enum_device()
{
    memset(&m_stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE|MV_USB_DEVICE, &m_stDeviceList);
    if(!check(nRet,"enum device"))return 0;
    
    if (m_stDeviceList.nDeviceNum > 0)
    {
        for (int i = 0; i < m_stDeviceList.nDeviceNum; i++)
        {
            std::cout<<"device "<<i<<std::endl;
            MV_CC_DEVICE_INFO* pDeviceInfo = m_stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo)
            {
                break;
            } 
            std::cout<<"DeviceTypeInfo:"<<pDeviceInfo->nDevTypeInfo<<std::endl;          
        }  
    } 
    else
    {
        std::cout<<"Find No Devices!\n"<<std::endl;
        return 0;
    }
    return 1;
}

bool Camera::open(unsigned int index)
{
    if(index >= m_stDeviceList.nDeviceNum)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }

    int nRet = MV_CC_OpenDevice(m_handle);
    if(!Controller::check(nRet,"open"))
    {
        MV_CC_DestroyHandle(m_handle);
        m_handle=nullptr;
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

bool Camera::handle(unsigned int index)
{
    if(index >= m_stDeviceList.nDeviceNum)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }
    
    int nRet = MV_CC_CreateHandle(&m_handle, m_stDeviceList.pDeviceInfo[index]);
    if(!Controller::check(nRet,"creat handle"))return false;

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
    MV_CC_GetFloatValue(m_handle, strNodeName , &exposure_time_value);

    int nRet = MV_CC_SetFloatValue(m_handle, strNodeName, exposure_time);
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
    MV_CC_GetFloatValue(m_handle, strNodeName , &gain_value);

    int nRet = MV_CC_SetFloatValue(m_handle, strNodeName, gain);
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

    MV_CC_SetEnumValue(m_handle, "TriggerMode", 0); 

    MV_CC_SetImageNodeNum(m_handle, 5);

    int nRet = MV_CC_StartGrabbing(m_handle);
    if (!Controller::check(nRet, "StartGrabbing"))
    {
        MV_CC_DestroyHandle(m_handle);
        m_handle=nullptr;
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

    int nRet = MV_CC_StopGrabbing(m_handle);
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

    int nRet = MV_CC_GetImageBuffer(m_handle, &stImageInfo, timeout);
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

    MV_CC_FreeImageBuffer(m_handle, &stImageInfo);

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
        nRet = MV_CC_CloseDevice(m_handle);
        Controller::check(nRet,"close");
        m_open=false;
    }
    if(m_handle != nullptr)
    {
        nRet = MV_CC_DestroyHandle(m_handle);
        Controller::check(nRet,"destroyhandle");
        m_handle=nullptr;
    }
    
    memset(&m_stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    std::cout << "Camera closed" << std::endl;

    return true;
}
