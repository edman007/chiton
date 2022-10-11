{config_load file="main.conf" section="camera"}
{include file="header.tpl" title=Camera}

<div class="singleViewer">
{include file="player.tpl" video=$video_info}
</div>

<div class="camera_sidebar">
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

{include file="events.tpl" }
{/if}

</div>{* camera_sidebar *}

<div class="camera_below">
{if !empty($avail_days)}
<div onclick="toggleBlock(this, 'hidden_lock_div', 'Lock Video');" class="td_col">Lock Video</div>
<div id="hidden_lock_div" class="hidden">
<form method="post" action="camera.php?id={$camera_id}" class="timeFormInput">
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
</div>
{/if}

{if !empty($locked_videos)}
<br />
Locked Videos:<br />
<ul class="locked_vids">
{foreach from=$locked_videos item=lock name=LOCK_DATA}
<li>{$lock.start_txt} - {$lock.end_txt}
<div class="td_col" onclick="toggleBlock(this, 'locked_vid_unlock_{$smarty.foreach.LOCK_DATA.index}', 'Unlock:');">Unlock</div>
<span class="hidden" id="locked_vid_unlock_{$smarty.foreach.LOCK_DATA.index}">
<form action="camera.php?id={$camera_id}" method="post" class="timeFormInput">
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
</span>
<div class="td_col" onclick="toggleBlock(this, 'locked_vid_export_{$smarty.foreach.LOCK_DATA.index}', 'Export:');">Export</div>
<span class="hidden" id="locked_vid_export_{$smarty.foreach.LOCK_DATA.index}">
<form action="camera.php?id={$camera_id}" method="post" class="timeFormInput">
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
</span>
</li>
{/foreach}
</ul>
{/if}

{if !empty($exports)}
<h3>Exports</h3>
<ul class="exports">
{foreach from=$exports item=exp name=EXPORT_LIST}
<li>
{if $exp['progress'] != 100}{* Not started or in progress *}
{$exp.start_txt} - {$exp.end_txt} ({$exp['progress']}%)
{else}{* Completed *}
<a href="{$exp['url']}" download="{if !empty($camera_name)}{$camera_name|escape}{else}Camera {$camera_id}{/if} - {$exp.start_txt}">
{$exp.start_txt} - {$exp.end_txt}</a>
{/if}
<form action="camera.php?id={$camera_id}" method="post">
<input type="hidden" name="delete_export" value="1">
<input type="hidden" name="export_id" value="{$exp['id']}">
<input type="submit" value="Delete">
</form>
</li>
{/foreach}
</ul>
{/if}

</div>{* camera_below *}
{include file="footer.tpl"}
