#include "Camera.h"

#include <iostream>
#include <csignal>

#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>

int main()
{
    SDK sdk;
    sdk.initialize();

    Camera camera;
    if(!camera.enum_device())
    {
        sdk.finalize();
        return -1;
    }

    if(!camera.handle(0))
    {
        return -1;
    }

    if(camera.open(0))
    {
        camera.set_exposure_time(10000,camera);
        camera.set_gain(10, camera);

        if(camera.start_grabbing())
        {
            std::cout<<"start grabbing.Press q to quit"<<std::endl;
            while(true)
            {
                cv::Mat frame;
                frame = camera.get_frame(1000);
                if(!frame.empty())
                {
                    cv::imshow("frame",frame);
                }
                else
                {
                    std::cout<<"Image fetch time out."<<std::endl;
                }

                int key =cv::waitKey(1);
                if(key=='q'||key=='Q')
                {
                    std::cout<<"Exit"<<std::endl;
                    break;
                }
            }
            camera.stop_grabbing();
        }

        camera.close();
    }

    sdk.finalize();
    return 0;
}