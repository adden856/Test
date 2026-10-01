#include "Camera.h"

#include <iostream>
#include <csignal>
#include <atomic>

#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>

std::atomic<bool> g_running{true};
void signalHandler(int signum) 
{
    g_running = false; 
}

int main()
{
    signal(SIGINT, signalHandler);

    int nRet = MV_OK;
     
    nRet =MV_CC_Initialize(); 
    if(nRet != MV_OK)
    {
        std::cout<<"MV_CC_Initialize fail"<<std::endl;
        return -1;
    }

    Camera camera;
    if(!camera.enum_device())
    {
        MV_CC_Finalize();
        return -1;
    }

    if(camera.open(0))
    {
        camera.set_exposure_time(10000);
        camera.set_gain(10);

        if(camera.start_grabbing())
        {
            std::cout<<"start grabbing.Press q to quit"<<std::endl;
            while(g_running)
            {
                if(camera.get_frame(1000))
                {
                    cv::Mat frame;
                    camera.read_frame(frame);
                    if(!frame.empty())
                    {
                        cv::imshow("frame",frame);
                    }
                }
                else
                {
                    std::cout<<"Image fetch time out."<<std::endl;
                }

                int key =cv::waitKey(1);
                if(key=='q'||key=='Q')
                {
                    std::cout<<"Exit"<<std::endl;
                    g_running = false;
                }
            }
            camera.stop_grabbing();
        }

        camera.close();
    }

    nRet =  MV_CC_Finalize();
    if(nRet != MV_OK)
    {
        std::cout<<"MV_CC_Finalize fail"<<std::endl;
    }
    return 0;
}