{config_load file="main.conf" section="camera"}
{include file="header.tpl" title=Camera}

<div class="singleViewer">
{include file="player.tpl" video=$video_info}
</div>

{if !empty($camera_name)}
<h2>{$camera_name|escape}</h2>
{else}
<h2>Camera {$camera_id}</h2>
{/if}
Shift+Scroll: Zoom<br/>
Scroll: Skip<br/>

{if !empty($avail_days)}
<div class="dateSwitch">
<form method="get" action="camera.php">
<input type="hidden" value="{$camera_id}" name="id"/>
<select name="start">
{foreach name=date_select item=day from=$avail_days}
<option value="{$day['timestamp']}" {if !empty($day['selected'])}selected="selected"{/if}>{$day['long']}</option>
{/foreach}
<input type="submit" value="Go"/>
</select>

</form>
</div><br/>
{/if}

{include file="footer.tpl"}
