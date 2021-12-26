/**************************************************************************
 *
 *     This file is part of Chiton.
 *
 *   Chiton is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Chiton is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Chiton.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Copyright 2020 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */
#include "export.hpp"
#include "util.hpp"
#include "stream_unwrap.hpp"
#include "stream_writer.hpp"
#include "io/cfmp4.hpp"
#include <memory>

Export::Export(Database &db, Config &cfg, FileManager &fm) : db(db), cfg(cfg), g_fm(fm) {
    force_exit = false;
    export_in_progress = false;
    id = 0;
    reserved_bytes = 0;
}

Export::~Export(void){
    lock.lock();
    force_exit = true;
    if (runner.joinable()){
        runner.join();//always join holding the lock since it could be joined from different threads
    }
    lock.unlock();
}

bool Export::check_for_jobs(void){
    if (export_in_progress){
        return true;//return true because it's already in progress
    }

    lock.lock();
    //check if the job is running
    if (runner.joinable()){
        runner.join();
    }

    const std::string sql = "SELECT id, starttime, endtime, camera, path, progress FROM exports WHERE progress != 100 LIMIT 1";
    DatabaseResult *res = db.query(sql);
    if (res && res->next_row()){
        id = res->get_field_long(0);
        starttime = res->get_field_long(1);
        endtime = res->get_field_long(2);
        camera = res->get_field_long(3);
        path = std::string(res->get_field(4));
        progress = res->get_field_long(5);
        delete res;

        return start_job();
    }
    lock.unlock();
    return false;
}

//should always be called while holding the lock
bool Export::start_job(void){
    export_in_progress = true;
    force_exit = false;
    lock.unlock();
    runner = std::thread(&Export::run_job, this);
    return runner.joinable();
}

void Export::run_job(void){
    Util::set_low_priority();//reduce the thread priority

    camera_cfg.load_camera_config(camera, db);

    FileManager fm(db, camera_cfg);
    if (path != ""){
        fm.rm_file(path + std::to_string(id) + EXPORT_EXT);
        path = "";
    }

    //generate a path
    struct timeval start;
    Util::unpack_time(starttime, start);
    path = fm.get_export_path(id, camera, start);
    progress = 1;
    LINFO("Performing Export for camera " + std::to_string(camera));
    update_progress();

    //query all segments we need
    std::string sql = "SELECT id, path, endtime, extension, name, init_byte, start_byte, end_byte, codec FROM videos WHERE camera = " + std::to_string(camera) + " AND ( "
        "( endtime >= " + std::to_string(starttime) + "  AND endtime <= " + std::to_string(endtime) + "  ) "
        " OR (starttime >= " + std::to_string(starttime) + " AND starttime <= " + std::to_string(endtime) + " )) "
        " ORDER BY starttime ASC ";
    DatabaseResult *res = db.query(sql);
    if (!res){
        LWARN("Failed to query segments for export");
        export_in_progress = false;
        return;
    }

    StreamUnwrap in = StreamUnwrap(camera_cfg);
    StreamWriter out = StreamWriter(camera_cfg);
    out.change_path(path);

    long total_time_target = endtime - starttime;
    reserved_bytes = 0;
    bool output_opened = false;
    std::string prev_ext = "", prev_codec;
    while (res->next_row()){
        if (prev_ext == "" && prev_codec == ""){
            prev_ext = res->get_field(3);
            prev_codec = res->get_field(8);
        } else if (prev_ext != res->get_field(3) || prev_codec != res->get_field(8)){
            if (!split_export(res->get_field_long(0))){
                LWARN("Error Splitting Export");
            }
            out.close();
            progress = 100;
            update_progress();
            export_in_progress = false;
            return;
        }

        std::string segment = fm.get_path(res->get_field_long(4), res->get_field(1), res->get_field(3));
        LDEBUG("Exporting " + segment);
        //we control the input stream by replacing the video-url with the segment
        camera_cfg.set_value("video-url", segment);

        std::unique_ptr<IOWrapper> io;

        if (res->get_field(3) == ".ts"){
            //get the filesize of the segment and subtrack from reserved bytes
            reserve_space(fm, fm.get_filesize(segment));
            if (!in.connect()){
                //try the next one...
                continue;
            }
        } else if (res->get_field(3) == ".mp4") {
            reserve_space(fm, res->get_field_long(7) - res->get_field_long(6));
            io = std::unique_ptr<CFMP4>(new CFMP4(segment, res->get_field_long(5), res->get_field_long(6), res->get_field_long(7)));
            camera_cfg.set_value("video-url", io->get_url());

            if (!in.connect(*io.get())){
                //try the next one...
                continue;
            }

        } else {
            LWARN(res->get_field(3) + " is not supported for export");
            continue;
        }
        if (!output_opened){
            if (!out.copy_streams(in)){
                continue;
            }
            output_opened = out.open();
            if (!output_opened){
                //failed to open it, we need to bail
                force_exit = true;//this will cause the job to be retried, assuming the issue was maybe drive space?
                break;
            }
        }

        AVPacket pkt;
        while (in.get_next_frame(pkt)){
            out.write(pkt, in.get_stream(pkt));
            in.unref_frame(pkt);
        }
        in.close();
        if (force_exit){
            LWARN("Export was canceled due to shutdown");
            break;
        }

        //compute the progress
        if (res->get_field(2) != "NULL"){//do not compute the progress if we don't know how long this is
            long new_progress = 100 - (100*(endtime - res->get_field_long(2))) / total_time_target;
            if (new_progress > progress && new_progress < 100){//this can result in number over 100, we don't put 100 in until it's actually done
                progress = new_progress;
                update_progress();
            }
        }
    }
    out.close();
    if (!force_exit){
        //if we are force exiting then we didn't finish
        progress = 100;
        update_progress();
    }
    id = 0;
    export_in_progress = false;
}

bool Export::update_progress(){
    long affected = 0;
    std::string sql = "UPDATE exports SET progress = " + std::to_string(progress) + " WHERE id = " + std::to_string(id);
    DatabaseResult *res = db.query(sql, &affected, NULL);
    if (res){
        delete res;
    }
    return affected != 0;
}

bool Export::rm_export(int export_id){
    //rm the export
    lock.lock();
    if (id == export_id){
        force_exit = true;//but we do not need to wait
    }

    std::string sql = "SELECT path, id FROM exports WHERE id = " + std::to_string(export_id);
    DatabaseResult *res = db.query(sql);
    long del_count = 0;
    if (res){
        if (res->next_row()){
            std::string path = res->get_field(0) + res->get_field(1) + EXPORT_EXT;
            g_fm.rm_file(path);
            delete res;

            sql = "DELETE FROM exports WHERE id = " + std::to_string(export_id);
            res = db.query(sql, &del_count, NULL);

        }
        delete res;
    }
    lock.unlock();
    return del_count > 0;
}

void Export::reserve_space(FileManager &fm, long size){
    if (size <= 0){
        LINFO("reserve_space called with invalid size");
        return;
    }
    if (reserved_bytes < size){
        //reserve more bytes
        long req_bytes = 50*size;//should be 5min of video
        fm.reserve_bytes(req_bytes, camera);
        reserved_bytes += req_bytes;
    }
    reserved_bytes -= size;
}

bool Export::split_export(long seg_id){
    std::string query = "INSERT INTO exports (camera, path, starttime, endtime) "
        "SELECT e.camera, e.path, s.starttime + 1, e.endtime "
        "FROM exports AS e, videos AS s "
        "WHERE e.id = " + std::to_string(id) + " AND s.id = " + std::to_string(seg_id);
    long affected;
    DatabaseResult *res = db.query(query, &affected, NULL);
    delete res;
    bool ret = affected != 0;
    query = "UPDATE exports SET endtime = (SELECT starttime FROM videos WHERE id = " + std::to_string(seg_id) + ") WHERE id = " + std::to_string(id);
    res = db.query(query, &affected, NULL);
    delete res;
    return affected != 0 && ret;
}
