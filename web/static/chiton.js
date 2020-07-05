function loadSite(page){
    if (page == "camera" || page == "home"){
        loadPlayer();
    }
}
function loadPlayer(){
    if (Hls.isSupported()) {
        var vtargets = document.getElementsByTagName('video');
        for (var i = 0; i < vtargets.length; i++){
            loadHLS(vtargets[i]);
            loadShortcuts(vtargets[i]);
        }
    }    
    
}

function loadHLS(video){
    if (Hls.isSupported()) {
        // bind them together
        var hls = new Hls();
        hls.attachMedia(video);
        hls.on(Hls.Events.MEDIA_ATTACHED, function () {
            var source = video.getElementsByTagName("source");
            if (source.length >= 1){
                hls.loadSource(source[0].src);
                hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
                    playVideo(video, vcontrol);
                });
            }
        });
        var videoWrapper = video.parentElement;
        var videoViewPort = videoWrapper.parentElement;
        var vcontrol = videoViewPort.getElementsByClassName("vcontrol")[0];
        function lockViewPortSize(){
            videoViewPort.style.height = videoWrapper.scrollHeight + "px";
            hls.off(Hls.Events.FRAG_BUFFERED, lockViewPortSize);
        }
        hls.on(Hls.Events.FRAG_BUFFERED, lockViewPortSize);
    } else if (video.canPlayType('application/vnd.apple.mpegurl')) {
        video.src = videoSrc;
        video.addEventListener('loadedmetadata', function() {
            video.play();
            playVideo(video, vcontrol);
            //lock in the viewport height
            videoViewPort.style.height = videoWrapper.offsetHeight + "px";
        });
    }
}
    
    
    
function loadShortcuts(video){
    //get the parent div
    var videoWrapper = video.parentElement;
    var videoViewPort = videoWrapper.parentElement;
    var startWidth = videoWrapper.offsetWidth;
    var maxWidth = startWidth*8;
    var lastOffsetX = 0;
    var lastOffsetY = 0;
    var vcontrol = videoViewPort.getElementsByClassName("vcontrol")[0];
    loadControls(video, vcontrol);

    //these are callbacks to allow zooming and panning of the video
    function applyOffSet(node, newX, newY){
        var maxDeltaX = videoWrapper.offsetWidth - videoViewPort.offsetWidth;
        var maxDeltaY = videoWrapper.offsetHeight - videoViewPort.offsetHeight;

        if (newX > maxDeltaX){
            newX = maxDeltaX;
        } else if (newX < -maxDeltaX) {
            newX = -maxDeltaX;
        }
        
        if (newY > maxDeltaY){
            newY = maxDeltaY;
        } else if (newY < -maxDeltaY) {
            newY = -maxDeltaY;
        }
        lastOffsetY = newY;
        lastOffsetX = newX;
        node.style.left = newX + 'px';
        node.style.top = newY + 'px';
    }
    var mouseWheelF = (ev) => {
        if (ev.shiftKey){//zoom
            var newWidth;
            var newHeight;
            var oldWidth = videoWrapper.offsetWidth;
            var oldHeight = videoWrapper.offsetHeight;
            
            if (ev.deltaY < 0){
                newWidth = videoWrapper.offsetWidth*1.5;
                if (newWidth > maxWidth){
                    newWidth = maxWidth;
                }
            } else {
                newWidth = videoWrapper.offsetWidth*0.75;
                if (newWidth < startWidth){
                    newWidth = startWidth;
                }
                
            }
            videoWrapper.style.width = newWidth + 'px';
            newHeight = videoWrapper.offsetHeight;
            //and fixup the offsets
            if (ev.deltaY < 0){
                applyOffSet(videoWrapper, lastOffsetX - ev.offsetX/2 , lastOffsetY  - ev.offsetY/2);
            } else {
                applyOffSet(videoWrapper, lastOffsetX*(newWidth/oldWidth) ,lastOffsetY*(newHeight/oldHeight));
            }
        } else {
            //media player shift
        }
    
        ev.preventDefault();
        return false;
    };
    var mousemoveF = (ev) => {
        var newX = lastOffsetX + ev.movementX/2;
        var newY = lastOffsetY + ev.movementY/2;
        applyOffSet(videoWrapper, newX, newY);
        ev.preventDefault();
        return false;
    };
    var mouseupF = (e) => {
        document.removeEventListener('mousemove', mousemoveF);
        document.removeEventListener('mouseup', mouseupF);
        e.preventDefault();
        return false;
    };

    var mousedownF = (e) => {
        document.addEventListener('mouseup', mouseupF, false);
        document.addEventListener('mousemove', mousemoveF, false);
        e.preventDefault();
        return false;
    };
    
    videoViewPort.addEventListener('mousewheel', mouseWheelF, false);
    videoWrapper.addEventListener('mousedown', mousedownF, false);

    //play/pause callbacks
    vcontrol.getElementsByClassName("playbtn")[0].addEventListener('click', (ev) => {playVideo(video, vcontrol);}, false);
    vcontrol.getElementsByClassName("pausebtn")[0].addEventListener('click', (ev) => {pauseVideo(video, vcontrol);}, false);

    //callback to manage switching out of fullscreen both through the exif fullscreen or if the browser forced it
    function clearFS(){
        if (!document.fullscreenElement){
            vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', enterFS, false);
            vcontrol.getElementsByClassName("fullscreen")[0].removeEventListener('click', exitFS);
            document.onFullscreenChange = null;
        }
    }
    
    var exitFS;
    var enterFS = (e) => {
        goFullscreen(video, vcontrol);
        vcontrol.getElementsByClassName("fullscreen")[0].removeEventListener('click', enterFS);
        vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', exitFS, false);
        document.onfullscreenchange = clearFS;
    };

    var exitFS = (e) => {
        clearFS();
        exitFullscreen(video, vcontrol);
    };

    //add fullscreen callback
    vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', enterFS, false);


    //load the video timestamps
    loadVideoTS(video, vcontrol);
}



function loadControls(video, vcontrol){
    //delete the video controls
    video.controls = false;
    vcontrol.classList.remove("hidden");
    
}

//play video, and update controls if successful
function playVideo(video, vcontrol){    
    var promise = video.play();
    if (promise !== undefined) {
        promise.then(_ => {
            //playing?
            vcontrol.getElementsByClassName("playbtn")[0].classList.add("hidden");
            vcontrol.getElementsByClassName("pausebtn")[0].classList.remove("hidden");
        }).catch(error => {
            // Autoplay was prevented.
            // console.log("Denied?");
            // Show a "Play" button so that user can start playback.
        });
    }
    
}

function pauseVideo(video, vcontrol){
    var promise = video.pause();
    vcontrol.getElementsByClassName("playbtn")[0].classList.remove("hidden");
    vcontrol.getElementsByClassName("pausebtn")[0].classList.add("hidden");
    
}

//brings a video fullscreen
function goFullscreen(video, vcontrol){
    if (!document.fullscreenElement) {
        vcontrol.parentElement.requestFullscreen();
    }
    
}

//exits fullscreen
function exitFullscreen(video, vcontrol){
    if (document.exitFullscreen){
        document.exitFullscreen(); 
    }
}

//loads the timestamp bar and controls
function loadVideoTS(video, vcontrol){
    var jsonData = Array();
    var fullDay = 3600*24;
    var totalGaps = 0;
    
    //query the camera ID and starttime
    var camera = parseInt(vcontrol.getElementsByClassName("cameraid")[0].innerHTML);
    var start_ts = parseInt(vcontrol.getElementsByClassName("starttime")[0].innerHTML);
    var pointer = null;

    var pointerMoved = false;
    var pointerDelta = 0;
    var pointerStartPos = 0;
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
        var newTS = convertToTS(newPos/progressBar.offsetWidth, jsonData);
        video.currentTime = newTS;
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
        var newTS = convertToTS(newPos/progressBar.offsetWidth, jsonData);
        video.currentTime = newTS;
        ev.preventDefault();
        ev.stopPropagation();
    }
    
    //query the info...
    getCameraInfo(camera, start_ts, (jdata) => {
        jsonData = jdata;        
        //console.log(jsonData);
        totalGaps = drawGaps(camera, vcontrol, jsonData);
        pointer = vcontrol.getElementsByClassName("cursor")[0]
        pointer.addEventListener('mousedown', cursorDragEnable, false);
        pointer.addEventListener('click', cursorClick, false);
    });

    var tsBox = vcontrol.getElementsByClassName("tsbox")[0];
    tsBox.innerHTML = getTSHTML(0, 3600*24);

    var progressBar = vcontrol.getElementsByClassName("progress")[0];
    
    video.addEventListener('timeupdate', (ev) => {
        var actualTime = convertTSToTime(video.currentTime, jsonData);
        var actualDuration = convertTSToTime(video.duration, jsonData)
        tsBox.innerHTML = getTSHTML(actualTime, actualDuration);
        var barWidth = progressBar.offsetWidth;
        var newEnd = (actualDuration/fullDay*barWidth);

        if (vcontrol.getElementsByClassName("future").length == 1){
            vcontrol.getElementsByClassName("future")[0].style.left =  newEnd - totalGaps+"px";
            vcontrol.getElementsByClassName("future")[0].style.width =  (barWidth - newEnd)+"px";
        }
        if (pointer !== null){
            pointer.style.left = (actualTime/fullDay*barWidth) - pointer.offsetWidth/2 + "px";
        }
    });

    progressBar.addEventListener('click', (ev)=>{
        var offset = ev.offsetX;
        if (ev.target !== progressBar){
            offset += ev.target.offsetLeft;
        }
        var requestedTime = convertToTS(offset/progressBar.offsetWidth, jsonData);
        video.currentTime = requestedTime;
    }, true);
    
}

function getCameraInfo(camera, ts, cbk){
    var url = 'info.php?start=' + ts + '&id=' + camera;

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

function drawGaps(camera, vcontrol, jsonData){
    var bar = vcontrol.getElementsByClassName("progress")[0];
    var width = bar.offsetWidth;
    var fullDay = 3600*24;
    var newHTML = "";
    var gapWidth = (jsonData.offset / fullDay) * width;
    var total_offset = gapWidth;

    newHTML += '<div class="gap" style="width:' + gapWidth+ 'px; left:0px;"></div>';
    for (var i = 0; i < jsonData.gaps.length; i++){
        if (i == 0 && jsonData.gaps[i].len > jsonData.offset){
            continue;//do not double count the offset if the video is discontinous over midnight
        }
        gapWidth = (jsonData.gaps[i].len / fullDay) * width;
        if (gapWidth <= 1){
            gapWidth = 1;
        }
        var left = (jsonData.gaps[i].start_ts / fullDay) * width - total_offset;
        total_offset += gapWidth;
        newHTML += '<div class="gap" style="width:' + gapWidth+ 'px; left:'+ left+';"></div>';
    }
    newHTML += '<div class="future"></div>';

    //the current cursor
    newHTML += '<div class="cursor"></div>';
    bar.innerHTML = newHTML;
    return total_offset;
}

//compute a TimeStamp from a float (0-1) representing the period displayed
function convertToTS(clickFraction, jsonData){
    var fullDay = 3600*24;
    var targetTime = clickFraction*fullDay;
    var totalGaps = jsonData.offset;
    if (totalGaps === undefined){
        totalGaps = 0;
    }
    if (jsonData.gaps === undefined){
        return targetTime - totalGaps;
    }
    for (var i; i < jsonData.gaps.length; i++){
        if (jsonData.gaps[i].actual_start_ts < targetTime){
            totalGaps += jsonData.gaps[i].len;
        } else {
            //only works if we guarentee gaps is in order, which it should be
            break;
        }
    }
    return targetTime - totalGaps;
}

//convert a video timestamp to Time
function convertTSToTime(ts, jsonData){
    var fullDay = 3600*24;
    var totalGaps = jsonData.offset;
    if (totalGaps === undefined){
        totalGaps = 0;
    }
    if (jsonData.gaps === undefined){
        return ts + totalGaps;
    }
    for (var i; i < jsonData.gaps.length; i++){
        if (jsonData.gaps[i].actual_start_ts < ts){
            totalGaps += jsonData.gaps[i].len;
        } else {
            //only works if we guarentee gaps is in order, which it should be
            break;
        }
    }
    return ts + totalGaps;


}

function getTSHTML(ts, len){
    return tsToStr(ts) + " / " + tsToStr(len);
}

function tsToStr(ts){
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