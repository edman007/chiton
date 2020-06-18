{config_load file="main.conf" section="home"}
{include file="header.tpl" title=Home}

{foreach from=$video_info item=video name=player_loop}
         <div class="tiledViewer"><a href="camera.php?id={$video.camera}">Camera {$video.camera}</a><br/>{include file="player.tpl" video=$video}</div>
{/foreach}

{include file="footer.tpl"}
