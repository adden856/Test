#include <opencv2/core/cuda.hpp>
#include <opencv2/opencv.hpp>

class Camera;

class Controller
{
public:
    bool enum_device();

    static bool check(int nRet,std::string msg);
};

class SDK
{
public:
    bool initialize();
    bool finalize();
};

class Camera
{
public:
    Camera();
    ~Camera();
    
    bool handle(unsigned int index);

    bool open(unsigned int index);
    bool close();

    bool get_open()const;
    bool get_grabbing()const;
    
    bool start_grabbing();
    bool stop_grabbing();

    void set_exposure_time(double exposure_time);

    void set_gain(double gain);

    cv::Mat get_frame(int timeout);

private:
    void* m_handle;
    bool m_open;
    bool m_grabbing;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
};
