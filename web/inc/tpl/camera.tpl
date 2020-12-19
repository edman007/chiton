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

{if !empty($avail_days)}
<br />
<form method="post" action="camera.php?id={$camera_id}">
Lock Video:<br />
From: <select name="from_d">
{foreach name=date_select item=day from=$avail_days}
<option value="{$day['timestamp']}" {if !empty($day['selected'])}selected="selected"{/if}>{$day['long']}</option>
{/foreach}
<input type="text" name="from_h" size="2" maxlength="2"  />:<input type="text" name="from_m" size="2" maxlength="2" />:<input type="text" name="from_s" size="2" maxlength="2"  />
<br />
To: <select name="to_d">
{foreach name=date_select item=day from=$avail_days}
<option value="{$day['timestamp']}" {if !empty($day['selected'])}selected="selected"{/if}>{$day['long']}</option>
{/foreach}
 <input type="text" name="to_h" size="2" maxlength="2" />:<input type="text" name="to_m" size="2" maxlength="2"  />:<input type="text" name="to_s" size="2" maxlength="2"  />
 <br />
 <input type="submit" value="Lock Video"/>
</form>



{/if}

{if !empty($locked_videos)}
<br />
Locked Videos:<br />
<ul>
{foreach from=$locked_videos item=lock name=LOCK_DATA}
<li>{$lock.start_txt} - {$lock.end_txt}
<form action="camera.php?id={$camera_id}" method="post">
<input type="hidden" name="unlock_segment" value="1">
{$lock.start_day_txt} <input type="hidden" name="start_day_ts" value="{$lock.start_day_ts}"/>
<input type="text" name="start_h" value="{$lock.start_h}" size="2"/> :
<input type="text" name="start_m" value="{$lock.start_m}" size="2"/> :
<input type="text" name="start_s" value="{$lock.start_s}" size="2"/> -
{$lock.end_day_txt} <input type="hidden" name="end_day_ts" value="{$lock.end_day_ts}"/>
<input type="text" name="end_h" value="{$lock.end_h}" size="2"/> :
<input type="text" name="end_m" value="{$lock.end_m}" size="2"/> :
<input type="text" name="end_s" value="{$lock.end_s}" size="2"/>
<input type="submit" value="Unlock"/>
</form>
<form action="camera.php?id={$camera_id}" method="post">
<input type="hidden" name="export" value="1">
{$lock.start_day_txt} <input type="hidden" name="start_day_ts" value="{$lock.start_day_ts}"/>
<input type="text" name="start_h" value="{$lock.start_h}" size="2"/> :
<input type="text" name="start_m" value="{$lock.start_m}" size="2"/> :
<input type="text" name="start_s" value="{$lock.start_s}" size="2"/> -
{$lock.end_day_txt} <input type="hidden" name="end_day_ts" value="{$lock.end_day_ts}"/>
<input type="text" name="end_h" value="{$lock.end_h}" size="2"/> :
<input type="text" name="end_m" value="{$lock.end_m}" size="2"/> :
<input type="text" name="end_s" value="{$lock.end_s}" size="2"/>
<input type="submit" value="Export"/>
</form>

</li>
{/foreach}
</ul>
{/if}
{include file="footer.tpl"}
