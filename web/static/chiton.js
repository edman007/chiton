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
 *   Copyright 2020-2022 Ed Martin <edman007@edman007.com>
 *
 **************************************************************************
 */

var cameraList = new Array();
var cameraOptions = new Map();
const audioLevels = ["high", "medium", "low", "mute"];

//check the hash, parse out start times
//This is the format used #X,YZ[,YZ...][:X,YZ...]
//X is the camera number
//Y is the option
//Z is the option value
//Current options:
// s -> start time of the player
function parseURLHash(){
    if (window.location.hash){
        const camera_sets = window.location.hash.substr(1).split(":");
        for (let opt of camera_sets){
            let opt_list = opt.split(',');
            if (opt_list.length > 1){
                let cam_num = parseInt(opt_list[0]);
                if (!isNaN(cam_num)){
                    let final_opts = new Map();
                    for (let i = 1; i < opt_list.length; i++){
                        let param = opt_list[i].substr(0,1);
                        let val = opt_list[i].substr(1);
                        switch (param){
                        case "s":
                            let stime = parseInt(val);
                            if (!isNaN(stime)){
                                final_opts.set("s", stime);
                            }
                            break;
                        default:
                            console.log("Unknown Hash Param " + param);
                        }
                    }
                    cameraOptions.set(cam_num, final_opts);
                }
            }
        }
    }
}

function loadSite(page){
    if (page == "camera" || page == "home"){
        loadPlayer();
    }
    if (page == "camera"){
        initEventScroll(cameraList[0].camera);
    }
}

function loadPlayer(){
    parseURLHash();
    var vtargets = document.getElementsByTagName('video');
    for (var i = 0; i < vtargets.length; i++){
        cameraList.push(new CameraState(vtargets[i]));
    }    
}

class CameraState {

    //html elements for the camera
    video;
    videoWrapper;
    videoViewPort;
    vcontrol;

    //camera metadata
    camera;
    start_ts;

    initialStart;//when positive, the camera is initilized to this start time

    //the data for the camera
    jsonData = Array();


    //video is the reference to the video tag
    constructor(video){
        this.video = video;
        this.videoWrapper = this.video.parentElement;
        this.videoViewPort = this.videoWrapper.parentElement;
        this.vcontrol = this.videoViewPort.getElementsByClassName("vcontrol")[0];
        this.camera = parseInt(this.vcontrol.getElementsByClassName("cameraid")[0].innerHTML);
        this.start_ts = parseInt(this.vcontrol.getElementsByClassName("starttime")[0].innerHTML);

        this.initialStart = 0;
        let camOpts = cameraOptions.get(this.camera);
        if (camOpts !== undefined){
            if (camOpts.has("s")){
                this.initialStart = camOpts.get("s");
            }
        }
        this.loadHLS();
        this.loadShortcuts();
    }

    static getCam(id){
        for (var i of cameraList){
            if (i.getId() == id){
                return i;
            }
        }
        return null;
    }

    getId(){
        return this.camera;
    }
    //also loads the pinchZoom function after the video load
    loadHLS(){
        var pinchZ;
        if (Hls.isSupported()) {
            // bind them together
            var cfg = {
                "debug": false,
                "enableWorker": true,
                "lowLatencyMode": true,
                "liveSyncDurationCount": 3,
                /* "maxFragLookUpTolerance": true, */
            };
            var hls = new Hls(cfg);
            hls.attachMedia(this.video);
            let cam = this;

            hls.on(Hls.Events.MEDIA_ATTACHED, function () {
                let source = cam.video.getElementsByTagName("source");
                if (source.length >= 1){
                    hls.loadSource(source[0].src);
                    hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
                        cam.initVolume();
                        cam.playVideo();
                    });
                }
            });
            function lockViewPortSize(){
                cam.videoViewPort.style.height = cam.videoWrapper.scrollHeight + "px";
                hls.off(Hls.Events.FRAG_BUFFERED, lockViewPortSize);

            }
            hls.on(Hls.Events.FRAG_BUFFERED, lockViewPortSize);
        } else if (this.video.canPlayType('application/vnd.apple.mpegurl')) {
            this.video.src = videoSrc;
            let cam = this;
            this.video.addEventListener('loadedmetadata', function() {
                cam.initVolume();
                cam.playVideo();
                //lock in the viewport height
                cam.videoViewPort.style.height = cam.videoWrapper.offsetHeight + "px";
            });
        }
    }

    loadShortcuts(){
        //get the parent div
        var startWidth = this.videoWrapper.offsetWidth;
        var maxWidth = startWidth*8;
        var lastOffsetX = 0;
        var lastOffsetY = 0;

        var touchStartX = 0;
        var touchStartY = 0;
        var swipeProcessed = false;


        this.loadControls();

        var mouseWheelF = (ev) => {
            if (!ev.shiftKey){
                ev.preventDefault();
                ev.stopPropagation();
                //media player shift
                if (ev.deltaY > 0){
                    jumpTime(60);//FIXME: Should be a config variable
                } else {
                    jumpTime(-60);
                }
            }
            return false;
        };
        let cam = this;
        this.videoViewPort.addEventListener('mousewheel', mouseWheelF, true);

        //play/pause callbacks
        this.vcontrol.getElementsByClassName("playbtn")[0].addEventListener('click', (ev) => {cam.playVideo();}, false);
        this.vcontrol.getElementsByClassName("pausebtn")[0].addEventListener('click', (ev) => {cam.pauseVideo();}, false);

        //volume callbacks
        var volButtons = this.vcontrol.getElementsByClassName("volume");
        for (var vb of volButtons){
            vb.addEventListener('click', (ev) => {cam.cycleVolume();}, true);
        }

        //callback to manage switching out of fullscreen both through the exif fullscreen or if the browser forced it
        function clearFS(){
            if (!document.fullscreenElement){
                cam.vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', enterFS, false);
                cam.vcontrol.getElementsByClassName("fullscreen")[0].removeEventListener('click', exitFS);
                document.onFullscreenChange = null;
            }
        }

        var exitFS;
        var enterFS = (e) => {
            cam.goFullscreen();
            cam.vcontrol.getElementsByClassName("fullscreen")[0].removeEventListener('click', enterFS);
            cam.vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', exitFS, false);
            document.onfullscreenchange = clearFS;
        };

        var exitFS = (e) => {
            clearFS();
            cam.exitFullscreen();
        };

        //add fullscreen callback
        this.vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', enterFS, false);

        //load the video timestamps
        this.loadVideoTS();

        function jumpTime(seconds){
            var target = cam.video.currentTime + seconds;
            if (target < 0){
                cam.video.currentTime = 0;
            } else if (target > cam.video.duration){
                cam.video.currentTime = cam.video.currentTime - 6;//FIXME: the 6 should be a config variable
            } else {
                cam.video.currentTime = target;
            }
            if (cam.video.paused || cam.video.seeking){
                cam.playVideo();
            }

        }

        //mobile functions...

        //swipe is left/right
        var touchStartF = (e) => {
            touchStartX = e.touches[0].clientX;
            touchStartY = e.touches[0].clientY;
            swipeProcessed = false;
        };

        var touchEndF = (e) => {
            if (swipeProcessed){
                return;
            }
            var deltaX = e.changedTouches[0].clientX - touchStartX;
            var deltaY = e.changedTouches[0].clientY - touchStartY;
            if (Math.abs(deltaX) > Math.abs(deltaY)){
                if (deltaX > 0){
                    //swipe right
                    jumpTime(+60);//FIXME: the 60 should be a config variable
                } else {
                    //swipe left
                    jumpTime(-60);
                }
            }
            swipeProcessed = true;

        };

        this.vcontrol.addEventListener('touchstart', touchStartF, false);
        this.vcontrol.addEventListener('touchmove', touchEndF, false);
    }

    



    loadControls(){
        //delete the video controls
        this.video.controls = false;
        this.vcontrol.classList.remove("hidden");
    }

    //play video, and update controls if successful
    playVideo(){
        var promise = this.video.play();
        if (promise !== undefined) {
            promise.then(_ => {
                //playing?
                this.vcontrol.getElementsByClassName("playbtn")[0].classList.add("hidden");
                this.vcontrol.getElementsByClassName("pausebtn")[0].classList.remove("hidden");
            }).catch(error => {
                // Autoplay was prevented.
                // console.log("Denied?");
                // Show a "Play" button so that user can start playback.
            });
        }
    }

    pauseVideo(){
        var promise = this.video.pause();
        this.vcontrol.getElementsByClassName("playbtn")[0].classList.remove("hidden");
        this.vcontrol.getElementsByClassName("pausebtn")[0].classList.add("hidden");
    }



    //cycles between the volume levels of the volume
    cycleVolume(){
        var vol = this.getVolumeLevelIdx();
        var newVol = (vol + 1) % audioLevels.length;//pick the next one
        //make the update
        var volControl = this.vcontrol.getElementsByClassName("volume");
        for (var e of volControl){
            if (e.classList.contains("volume_" + audioLevels[vol])){
                e.classList.add("hidden");
            }
            if (e.classList.contains("volume_" + audioLevels[newVol])){
                e.classList.remove("hidden");
            }
        }
        this.setVolume(newVol);
    }

    //set the volume to one of the Indexes of audioLevels
    setVolume(level){
        //and set the new volume
        switch (level){
        case 0:
            this.video.volume = 1.0;
            break;
        case 1:
            this.video.volume = 0.67;
            break;
        case 2:
            this.video.volume = 0.33;
            break;
        case 3:
            this.video.volume = 0;
            break;
        default:
            console.log('Unsupported Volume');
        }
    }

    //checks the current playback volume and returns the index of audioLevels corrasponding to it
    getVolumeLevelIdx(){
        var vol = this.video.volume;
        if (vol > 0.95){
            return 0;//high (1.0)
        } else if (vol > 0.4) {
            return 1;//medium (0.67)
        } else if (vol > 0) {
            return 2;//low (0.33)
        } else {
            return 3;//mute (0)
        }
    }

    //checks the HTML for the default volume and selects that
    initVolume(){
        for (var i = 0; i < audioLevels.level; i++){
            var el = this.vcontrol.getElementsByClassName("volume_" + audioLevels[i]);
            if (el.classList.contains("hidden")){
                this.setVolume(i);
                return i;
            }
        }
        this.setVolume(3);
        return 3;//shouldn't get here, pick mute
    }
    //brings a video fullscreen
    goFullscreen(){
        if (!document.fullscreenElement) {
            this.vcontrol.parentElement.requestFullscreen();
        }
    }

    //exits fullscreen
    exitFullscreen(){
        if (document.exitFullscreen){
            document.exitFullscreen();
        }
    }

    //loads the timestamp bar and controls
    loadVideoTS(){
        var fullDay = 3600*24;
        var totalGaps = 0;
        var vcontrol_box = this.vcontrol.getElementsByClassName("control_box")[0];

        var pointer = null;

        var pointerMoved = false;
        var pointerDelta = 0;
        var pointerStartPos = 0;
        let cam = this;
        function cursorDragEnable(ev){
            pointerMoved = false;
            pointerDelta = 0;
            pointerStartPos = pointer.offsetLeft + ev.offsetX;
            document.addEventListener('mousemove', cursorDrag, false);
            document.addEventListener('mouseup', cursorDragDisable, false);
            ev.preventDefault();
            return false;
        }

        function cursorDrag(ev){
            pointerMoved = true;
            pointerDelta += ev.movementX;
            var newPos = pointerStartPos + pointerDelta;
            var newTS = cam.convertToTS(newPos/progressBar.offsetWidth);
            cam.video.currentTime = newTS;
            if (cam.video.paused || can.video.seeking){
                cam.playVideo();
            }
            ev.preventDefault();
        }

        function cursorDragDisable(ev){
            document.removeEventListener('mousemove', cursorDrag);
            document.removeEventListener('mouseup', cursorDragDisable);
            if (pointerMoved == false){

            }
            ev.preventDefault();
            return false;
        }

        function cursorClick(ev){
            var newPos = pointer.offsetLeft + ev.offsetX;
            var newTS = cam.convertToTS(newPos/progressBar.offsetWidth);
            cam.video.currentTime = newTS;
            if (cam.video.paused || cam.video.seeking){
                cam.playVideo();
            }

            ev.preventDefault();
            ev.stopPropagation();
        }


        //query the info...
        this.getCameraInfo((jdata) => {
            cam.jsonData = jdata;
            //console.log(jsonData);
            totalGaps = cam.drawGaps();
            pointer = cam.vcontrol.getElementsByClassName("cursor")[0]
            pointer.addEventListener('mousedown', cursorDragEnable, false);
            pointer.addEventListener('click', cursorClick, false);

            //if this initialstart is positive, jump to it
            if (cam.initialStart > 0) {
                cam.jumpRealTime(cam.initialStart);
                cam.initialStart = -1;
            }
        });

        var tsBox = this.vcontrol.getElementsByClassName("tsBox")[0];
        tsBox.innerHTML = this.getTSHTML(0, 3600*24);

        var progressBar = this.vcontrol.getElementsByClassName("progress")[0];

        this.video.addEventListener('timeupdate', (ev) => {
            var actualTime = cam.convertTSToTime(cam.video.currentTime);
            var actualDuration = cam.convertTSToTime(cam.video.duration)
            tsBox.innerHTML = cam.getTSHTML(actualTime, actualDuration);
            var barWidth = progressBar.offsetWidth;
            var newEnd = (actualDuration/fullDay)*barWidth;

            if (cam.vcontrol.getElementsByClassName("future").length == 1){
                cam.vcontrol.getElementsByClassName("future")[0].style.width =  (barWidth - (newEnd))+"px";
            }
            if (pointer !== null){
                pointer.style.left = (actualTime/fullDay*barWidth) - pointer.offsetWidth/2 + "px";
            }
        });

        this.vcontrol.addEventListener('click', (ev)=>{
            var offset = ev.offsetX;
            if (ev.target === vcontrol_box || vcontrol_box.contains(ev.target)){
                return;//.control_box has all the buttons and we do not jump for clicks there
            }
            if (ev.target !== progressBar){
                offset += ev.target.offsetLeft;
            }

            var requestedTime = cam.convertToTS(offset/progressBar.offsetWidth);
            cam.video.currentTime = requestedTime;
            if (cam.video.paused || cam.video.seeking){
                cam.playVideo();
            }

        }, true);
    }

    getCameraInfo(cbk){
        var url = 'info.php?start=' + this.start_ts + '&id=' + this.camera;

        var xmlhttp = new XMLHttpRequest();
        xmlhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
                var data = JSON.parse(this.responseText);
                cbk(data);
            }
        };
        xmlhttp.open("GET", url, true);
        xmlhttp.send();
    }

    drawGaps(){
        var bar = this.vcontrol.getElementsByClassName("progress")[0];
        var width = bar.offsetWidth;
        var fullDay = 3600*24;
        var newHTML = "";
        var gapWidth = 0;
        var total_offset = 0;

        for (var i = 0; i < this.jsonData.gaps.length; i++){
            gapWidth = (this.jsonData.gaps[i].len / fullDay) * width;
            if (gapWidth <= 1){
                gapWidth = 1;
            }

            var left = (this.jsonData.gaps[i].start_ts / fullDay) * width  /*- total_offset*/;
            total_offset += gapWidth;
            newHTML += '<div class="gap" style="width:' + gapWidth+ 'px; left:'+ left+'px;"></div>';
        }
        newHTML += '<div class="future"></div>';

        //the current cursor
        newHTML += '<div class="cursor"></div>';
        bar.innerHTML = newHTML;
        return total_offset;
    }

    //compute a TimeStamp from a float (0-1) representing the period displayed
    convertToTS(clickFraction){
        var fullDay = 3600*24;
        var targetTime = clickFraction*fullDay;
        return this.convertTimeToTS(targetTime);
    }

    //compute a TimeStamp from a wall time
    convertTimeToTS(targetTime){
        var totalGaps = 0;
        if (this.jsonData.gaps === undefined){
            return targetTime;
        }

        for (var i = 0; i < this.jsonData.gaps.length; i++){
            if (this.jsonData.gaps[i].actual_start_ts < targetTime){
                totalGaps += this.jsonData.gaps[i].len;
            } else {
                //only works if we guarentee gaps is in order, which it should be
                break;
            }
        }

        return targetTime - totalGaps;
    }

    //convert a video timestamp to Time
    convertTSToTime(ts){
        var fullDay = 3600*24;
        var totalGaps = 0;

        if (this.jsonData.gaps === undefined){
            return ts;
        }

        for (var i = 0; i < this.jsonData.gaps.length; i++){
            if (this.jsonData.gaps[i].actual_start_ts < ts){
                totalGaps += this.jsonData.gaps[i].len;
            } else {
                //only works if we guarentee gaps is in order, which it should be
                break;
            }
        }
        return ts + totalGaps;
    }

    getTSHTML(ts, len){
        return this.tsToStr(ts) + " / " + this.tsToStr(len);
    }

    tsToStr(ts){
        if (isNaN(ts)){
            return "??";
        }
        var hours = Math.floor(ts / 3600);
        ts -= hours*3600;
        var min = Math.floor(ts / 60);
        ts -= min*60;

        //and zero pad...
        if (hours < 10){
            hours = "0" + hours;
        }
        if (min < 10) {
            min = "0" + min;
        }
        if (ts < 10){
            ts =  "0" + ts.toFixed(3);
        } else {
            ts = ts.toFixed(3);
        }
        return hours +":" + min + ":" + ts;
    }

    jumpRealTime(wallTime){
        var target = this.convertTimeToTS(wallTime - this.start_ts);
        if (target < 0){
            this.video.currentTime = 0;
        } else if (target > this.video.duration){
            this.video.currentTime = this.video.currentTime - 6;//FIXME: the 6 should be a config variable
        } else {
            this.video.currentTime = target;
        }
        if (this.video.paused || this.video.seeking){
            this.playVideo();
        }
    }
}

//Manages collapsable blocks
function toggleBlock(caller, target, nextName){
    document.getElementById(target).classList.toggle("hidden");
    caller.classList.toggle("td_exp");
    caller.classList.toggle("td_col");
    var oldName = caller.innerHTML;
    caller.innerHTML = nextName;
    caller.onclick=function(){toggleBlock(caller, target, oldName);};
}

var event_start = -1;
function initEventScroll(camera){
    let ev_ul = document.getElementsByClassName("events")[0];
    let ev_id_txt = ev_ul.lastElementChild.id.substr(5);
    let ev_id_arr = ev_id_txt.split('_');
    let cam = CameraState.getCam(camera);
    if (ev_id_arr.length == 2 && cam != null){
        ev_ul.onscroll = (e) => {
            let dist = e.target.scrollHeight-e.target.scrollTop-e.target.clientHeight;
            if (dist < 150){//when within 150px of the bottom reload
                ev_ul.onscroll = undefined;
                scrollEvents(camera, cam.start_ts, ev_id_arr[1], ev_id_arr[0]);
            }
        }
    }

}
function scrollEvents(camera,  cam_start, ev_start, last_ev_id){
    let url = 'events.php?start=' + cam_start + '&id=' + camera + '&ev_start=' + ev_start + '&ev_id=' + last_ev_id
    let xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementsByClassName("events")[0].innerHTML += this.responseText;
            if (this.responseText.length > 5){
                initEventScroll(camera);
            }
        }
    };
    xmlhttp.open("GET", url, true);
    xmlhttp.send();


}
