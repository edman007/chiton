{config_load file="main.conf" section="camera"}
{include file="header.tpl" title=Camera}

<div class="singleViewer">
{include file="player.tpl" video=$video_info[0]}
</div>

<script>
var cameraInfo = {ldelim}
{foreach from=$video_info item=video name=video_script}
{$video.id}: {ldelim} src: "{$video.url}",
                      prev: {if $smarty.foreach.video_script.last}-1{else}{$video_info[$smarty.foreach.video_script.index +1].id}{/if},
                      next: {if $smarty.foreach.video_script.first}-1{else}{$video_info[$smarty.foreach.video_script.index -1].id}{/if}
             {rdelim},
{/foreach}
{rdelim};
var selectedCam = cameraInfo[{$video_info[0].id}];
</script>


<ul class="vselector"">
{foreach from=$video_info item=video name=video_list}
<li><a name="cam_{$video.id}" href="#vid_{$video.id}" onclick="switchVideo({$video.id});">{$video.starttime}</a></li>
{/foreach}
</ul>

{include file="footer.tpl"}
