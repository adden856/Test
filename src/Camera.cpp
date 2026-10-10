#include "Camera.h"

#include <cstddef>
#include <iostream>

#include <opencv2/core/hal/interface.h>
#include <opencv2/core/mat.hpp>
#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>

MV_CC_DEVICE_INFO_LIST m_stDeviceList;


bool check(int nRet,std::string msg)
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
    if(check(nRet,"sdk intialize"))
    {
        return false;
    }
    return true;
}

bool SDK::finalize()
{
    int nRet = MV_CC_Finalize();
    if(check(nRet,"sdk finalize"))
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

bool enum_device()
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

bool Camera::handle(unsigned int index)
{
    if(index >= m_stDeviceList.nDeviceNum)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }
    
    int nRet = MV_CC_CreateHandle(&m_handle, m_stDeviceList.pDeviceInfo[index]);
    if(!check(nRet,"creat handle"))return false;

    return true;
}

bool Camera::open(unsigned int index)
{
    if(index >= m_stDeviceList.nDeviceNum)
    {
        std::cout<<"index out of range"<<std::endl;
        return false;
    }

    int nRet = MV_CC_OpenDevice(m_handle);
    if(!check(nRet,"open"))
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

void* Camera::get_handle()const
{
    void* handle = m_handle;
    return handle;
}

void set_exposure_time(double exposure_time,const Camera& a)
{
    bool m_open = a.get_open();
    bool m_grabbing = a.get_grabbing();
    void* m_handle = a.get_handle();
    if(m_open == false || m_grabbing == true)
    {
        std::cout<<"device not open or grabbing"<<std::endl;
        return;
    }

    const char* strNodeName = "ExposureTime";

    MVCC_FLOATVALUE exposure_time_value;
    MV_CC_GetFloatValue(m_handle, strNodeName , &exposure_time_value);

    int nRet = MV_CC_SetFloatValue(m_handle, strNodeName, exposure_time);
    check(nRet,"set exposure time");
}

void set_gain(double gain,const Camera& a)
{
    bool m_open = a.get_open();
    bool m_grabbing = a.get_grabbing();
    void* m_handle = a.get_handle();
    if(m_open == false || m_grabbing == true)
    {
        std::cout<<"device not open or grabbing"<<std::endl;
        return;
    }

    const char* strNodeName = "Gain";

    MVCC_FLOATVALUE gain_value;
    MV_CC_GetFloatValue(m_handle, strNodeName , &gain_value);

    int nRet = MV_CC_SetFloatValue(m_handle, strNodeName, gain);
    check(nRet,"set gain");
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
    if (!check(nRet, "StartGrabbing"))
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
    if (!check(nRet, "StopGrabbing"))
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
            check(nRet, "GetImageBuffer");
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
        check(nRet,"close");
        m_open=false;
    }
    if(m_handle != nullptr)
    {
        nRet = MV_CC_DestroyHandle(m_handle);
        check(nRet,"destroyhandle");
        m_handle=nullptr;
    }
    
    memset(&m_stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    std::cout << "Camera closed" << std::endl;

    return true;
}
