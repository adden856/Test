
#include <memory>
#include <opencv2/core/cuda.hpp>
#include <opencv2/opencv.hpp>

class Controller
{
    struct Impl;
    std::unique_ptr<Impl> m_impl;
public:
    Controller();
    ~Controller();

    Controller(const Controller&)=delete;
    Controller& operator=(const Controller&)=delete;

    bool enum_device()const;

    unsigned device_num()const;

    void* device_info(unsigned index)const;

    static bool check(int nRet,std::string msg);
};

class Camera
{
public:
    Camera();
    ~Camera();
    
    bool handle(Controller& ctrl,unsigned int index);

    bool open(Controller& ctrl,unsigned int index);
    bool close();

    bool get_open()const;
    bool get_grabbing()const;
    
    bool start_grabbing();
    bool stop_grabbing();

    void set_exposure_time(double exposure_time);

    void set_gain(double gain);

    cv::Mat get_frame(int timeout);

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

private:
    struct Deleter{void operator()(void* h)const;};

    std::unique_ptr<void,Deleter> m_handle {nullptr};
    bool m_open = 0;
    bool m_grabbing = 0;

};
