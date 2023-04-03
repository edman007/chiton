{config_load file="main.conf" section="home"}
{include file="header.tpl" title=Home}

{foreach from=$video_info item=video name=player_loop}
         {if empty($video.active)}
         {assign var="active_found" value="1"}
         {else}
         <div class="tiledViewer"><a href="camera.php?id={$video.camera}">{if !empty($video.name)}{$video.name|escape}{else}Camera {$video.camera}{/if}</a><br/>{include file="player.tpl" video=$video}</div>
         {/if}
{/foreach}

{if !empty($active_found)}
<h3 class="inactive">Inactive Cameras</h3>
{foreach from=$video_info item=video name=inactive_loop}
         {if empty($video.active)}
         <a href="camera.php?id={$video.camera}">{if !empty($video.name)}{$video.name|escape}{else}Camera {$video.camera}{/if}</a><br/>
         {/if}
{/foreach}
{/if}
{include file="footer.tpl"}
