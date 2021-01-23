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

Export::Export(Database &db, Config &cfg, FileManager &fm) : db(db), cfg(cfg), g_fm(fm) {
    force_exit = false;
    export_in_progress = false;
    id = 0;
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
    std::string sql = "SELECT id, path, endtime FROM videos WHERE camera = " + std::to_string(camera) + " AND ( "
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

    long reserved_bytes = 0;
    bool output_opened = false;
    while (res->next_row()){
        std::string segment = fm.get_path(res->get_field_long(0), res->get_field(1));
        LDEBUG("Exporting " + segment);
        //we control the input stream by replacing the video-url with the segment
        camera_cfg.set_value("video-url", segment);

        //get the filesize of the segment and subtrack from reserved bytes
        long seg_size = fm.get_filesize(segment);
        if (seg_size > 0){
            if (reserved_bytes < seg_size){
                //reserve more bytes
                long req_bytes = 50*seg_size;//should be 5min of video
                fm.reserve_bytes(req_bytes, camera);
                reserved_bytes += req_bytes;
            }
            reserved_bytes -= seg_size;
        }

        if (!in.connect()){
            //try the next one...
            continue;
        }

        if (!output_opened){
            if (!out.copy_streams(in)){
                continue;
            }
            output_opened = out.open();
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
