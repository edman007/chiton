function loadSite(page){
    if (page == "camera"){
        loadPlayerSelector();
    }
}

function switchVideo(cam){
    selectedCam = cameraInfo[cam];
    var player = document.getElementsByTagName("video")[0];
    var src = player.getElementsByTagName("source")[0];
    src.src = selectedCam.src;
    player.load();
    
    var promise = player.play();
    if (promise !== undefined) {
        promise.then(_ => {
            //playing?
        }).catch(error => {
            // Autoplay was prevented.
            // Show a "Play" button so that user can start playback.
        });
    }
}
function loadPlayerSelector(){
    if (window.location.hash.startsWith('#vid_')){
        var newCam = window.location.hash.substr(5);
        console.log(cameraInfo[newCam]);
        switchVideo(newCam);
    }

    /* This makes the next video auto-play */
    document.getElementsByTagName("video")[0].onended = function(){
        if (selectedCam.next != -1){
            switchVideo(selectedCam.next);
        }
    };
    
    
}
