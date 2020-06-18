function loadSite(page){
    if (page == "camera" || page == "home"){
        loadPlayer();
    }
}

function switchVideo(cam){
    selectedCam = cameraInfo[cam];
    var player = document.getElementsByTagName("video")[0];
    var src = player.getElementsByTagName("source")[0];
    src.src = selectedCam.src;
    player.load();
    
}
function loadPlayer(){
    if (Hls.isSupported()) {
        var vtargets = document.getElementsByTagName('video');
        for (var i = 0; i < vtargets.length; i++){
            loadHLS(vtargets[i]);
            var promise = vtargets[i].play();
            if (promise !== undefined) {
                promise.then(_ => {
                    //playing?
                }).catch(error => {
                    // Autoplay was prevented.
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

    
