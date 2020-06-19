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
            var promise = vtargets[i].play();
            if (promise !== undefined) {
                promise.then(_ => {
                    //playing?
                    //console.log("Playing?");
                }).catch(error => {
                    // Autoplay was prevented.
                    // console.log("Denied?");
                    // Show a "Play" button so that user can start playback.
                });
            }

        }
    }    
    
}

function loadHLS(video){
    var hls = new Hls();
    // bind them together
    hls.attachMedia(video);
    hls.on(Hls.Events.MEDIA_ATTACHED, function () {
        console.log("video and hls.js are now bound together !");
        var source = video.getElementsByTagName("source");
        if (source.length >= 1){
            hls.loadSource(source[0].src);
            hls.on(Hls.Events.MANIFEST_PARSED, function (event, data) {
                console.log("manifest loaded, found " + data.levels.length + " quality level");
            });
        }
    });
}

    

function loadShortcuts(video){
    //get the parent div
    var videoWrapper = video.parentElement;
    var videoViewPort = videoWrapper.parentElement;
    var startWidth = videoWrapper.offsetWidth;
    var maxWidth = startWidth*8;
    var lastOffsetX = 0;
    var lastOffsetY = 0;

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
        var newX = lastOffsetX + ev.movementX;
        var newY = lastOffsetY + ev.movementY;
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
}


