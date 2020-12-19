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
#include "file_manager.hpp"

Export::Export(Database &db, Config &cfg) : db(db), cfg(cfg) {
    force_exit = false;
    export_in_progress = false;
}

Export::~Export(void){
    force_exit = true;
    if (runner.joinable()){
        runner.join();
    }
}

bool Export::check_for_jobs(void){
    //check if the job is running
    if (!export_in_progress && runner.joinable()){
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
    return false;
}

bool Export::start_job(void){
    if (export_in_progress){
        return true;//we will refuse, but return true because it's already in progress
    }
    export_in_progress = true;
    runner = std::thread(&Export::run_job, this);
    return runner.joinable();
}

void Export::run_job(void){
    Util::set_low_priority();//reduce the thread priority
    LWARN("THREAD RUNNING");

    camera_cfg.load_camera_config(camera, db);

    FileManager fm = FileManager(db, camera_cfg);
    if (path != ""){
        fm.rm_file(path + std::to_string(id) + EXPORT_EXT);
        path = "";
    }

    //generate a path
    struct timeval start;
    Util::unpack_time(starttime, start);
    path = fm.get_export_path(id, camera, start);
    progress = 1;

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
    StreamWriter out = StreamWriter(camera_cfg, path, in);
    long total_time_target = endtime - starttime;


    while (res->next_row()){
        std::string segment = fm.get_path(res->get_field_long(0), res->get_field(1));
        LDEBUG("Exporting " + segment);
        //we control the input stream by replacing the video-url with the segment
        camera_cfg.set_value("video-url", segment);

        AVPacket pkt;
        in.connect();
        out.open();

        while (in.get_next_frame(pkt)){
            out.write(pkt);
            in.unref_frame(pkt);
        }
        in.close();
        if (force_exit){
            break;
        }

        //compute the progress
        long new_progress = (endtime - res->get_field_long(2)) / total_time_target;

        if (new_progress > progress && new_progress < 100){//this can result in number over 100, we don't put 100 in until it's actually done
            progress = new_progress;
            update_progress();
        }
    }
    out.close();
    progress = 100;
    update_progress();
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
