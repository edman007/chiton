{config_load file="main.conf" section="home"}
{include file="header.tpl" title=Home}

{foreach from=$video_info item=video name=player_loop}
         <div class="tiledViewer"><a href="camera.php?id={$video.camera}">{if !empty($video.name)}{$video.name|escape}{else}Camera {$video.camera}{/if}</a><br/>{include file="player.tpl" video=$video}</div>
{/foreach}

{include file="footer.tpl"}
