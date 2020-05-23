#include "camera.hpp"
#include "util.hpp"
#include "stream_writer.hpp"

Camera::Camera(int camera, Database& db) : id(camera), db(db), stream(cfg), fm(db, cfg) {
    //load the config
    load_cfg();
    //stream = StreamUnwrap(cfg);
}
Camera::~Camera(){

}

void Camera::load_cfg(void){
    //loads the global and then overwrites it with the local
    DatabaseResult *res = db.query("SELECT name, value FROM config WHERE camera IS NULL OR camera = " + std::to_string(id) + " ORDER by camera DESC" );
    while (res && res->next_row()){
        cfg.set_value(res->get_field(0), res->get_field(1));
    }
    delete res;
}
    
void Camera::run(void){
    stream.connect();
    long int file_id;
    struct timeval start;
    Util::get_videotime(start);
    std::string new_output = fm.get_next_path(file_id, id, start);
    
    StreamWriter out = StreamWriter(cfg, new_output, stream);
    out.open();

    AVPacket pkt;
    while (stream.get_next_frame(pkt)){
        out.write(pkt);//log it
        
        stream.unref_frame(pkt);
    }
    out.close();
    
}

void Camera::stop(void){

}

int Camera::ping(void){
    return 0;
}
