#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include "database.hpp"
#include "chiton_config.hpp"
#include "stream_unwrap.hpp"
#include "file_manager.hpp"

class Camera {

    /*
     * This monitors the Recording process for one camera
     */
    
public:
    Camera(int camera, Database& db);
    ~Camera();
    void run(void);//connect and run the camera monitor
    void stop(void);//requests the thread stops
    int ping(void);//checks that this is running, will always return within 1 second, returns 0 if thread is not stalled

private:

    void load_cfg(void);
    int id; //camera ID
    Config cfg;
    Database& db;
    StreamUnwrap stream;
    FileManager fm;
};
#endif
