#include <opencv2/core/cuda.hpp>
#include <opencv2/opencv.hpp>
#include <MvCameraControl.h>

class Camera
{
public:
    Camera();
    ~Camera();

    bool check(int nRet,std::string msg);
    
    bool open(unsigned int index);
    bool close();

    void set_exposure_time(double exposure_time);
    void set_gain(double gain);

    bool enum_device();

    bool start_grabbing();
    bool stop_grabbing();

    bool get_frame(int timeout);

    void read_frame(cv::Mat& frame_copy);

private:
    void* m_handle;
    bool m_open;
    bool m_grabbing;
    MV_CC_DEVICE_INFO_LIST m_stDeviceList;
    cv::Mat frame;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
};

