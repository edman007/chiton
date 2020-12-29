<div class="video_viewport">
<pinch-zoom class="video_wrapper" min-scale="1" max-scale="10" two-finger-pan="1">
<video controls>
<source src="{$video.url}"/>
Video is not supported by your browser
</video>
</pinch-zoom>

<div class="vcontrol hidden">
<div class="progress"></div>
<div class="control_box">
<div class="play"><img src="static/breeze/2x-playback-start.png" srcset="static/breeze/2x-playback-start.png, static/breeze/4x-playback-start.png 2x" alt="Play" class="playbtn" /><img src="static/breeze/2x-playback-pause.png" srcset="static/breeze/2x-playback-pause.png, static/breeze/4x-playback-pause.png 2x" alt="Pause" class="pausebtn hidden" /></div>
<div class="tsBox"></div>
<div class="fullscreen"><img src="static/breeze/2x-view-fullscreen.png" srcset="static/breeze/2x-view-fullscreen.png, static/breeze/4x-view-fullscreen.png 2x" alt="Fullscreen"/></div>
<div class="volume volume_high hidden"><img src="static/breeze/2x-audio-volume-high.png" srcset="static/breeze/2x-audio-volume-high.png, static/breeze/4x-audio-volume-high.png 2x" alt="Volume"/></div>
<div class="volume volume_medium hidden"><img src="static/breeze/2x-audio-volume-medium.png" srcset="static/breeze/2x-audio-volume-medium.png, static/breeze/4x-audio-volume-medium.png 2x" alt="Volume"/></div>
<div class="volume volume_low hidden"><img src="static/breeze/2x-audio-volume-low.png" srcset="static/breeze/2x-audio-volume-low.png, static/breeze/4x-audio-volume-low.png 2x" alt="Volume"/></div>
<div class="volume volume_mute"><img src="static/breeze/2x-audio-volume-muted.png" srcset="static/breeze/2x-audio-volume-muted.png, static/breeze/4x-audio-volume-muted.png 2x" alt="Volume"/></div>
</div>
{* Metadata for the javascript *}
<div class="cameraid hidden">{$video.camera}</div>
<div class="starttime hidden">{$video.start_ts}</div>
</div>

</div>