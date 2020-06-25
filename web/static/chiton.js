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
            console.log("video and hls.js are now bound together !");
            var source = video.getElementsByTagName("source");
            if (source.length >= 1){
                hls.loadSource(source[0].src);
                hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
                    console.log("manifest loaded, found " + data.levels.length + " quality level");
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
        console.log("was: " + videoWrapper.offsetLeft, ev.movementX);
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
    vcontrol.getElementsByClassName("playbtn")[0].addEventListener('click', (ev) => {playVideo(video, vcontrol);}, false);
    vcontrol.getElementsByClassName("pausebtn")[0].addEventListener('click', (ev) => {pauseVideo(video, vcontrol);}, false);

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
    vcontrol.getElementsByClassName("fullscreen")[0].addEventListener('click', enterFS, false);

    
}



function loadControls(video, vcontrol){
    //delete the video controls
    video.controls = false;
    vcontrol.classList.remove("hidden");
    
}

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

function goFullscreen(video, vcontrol){
    if (!document.fullscreenElement) {
        vcontrol.parentElement.requestFullscreen();
    }
    
}

function exitFullscreen(video, vcontrol){
    if (document.exitFullscreen){
        document.exitFullscreen(); 
    }
}
