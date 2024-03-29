{config_load file="main.conf" section="stats"}
{include file="header.tpl" title="Camera Stats"}
{if !isset($display_name)}
<h2>System Stats</h2>
{else}
<h2>{$display_name} Stats</h2>
{/if}

{if !empty($detailed)}
<ul class="detailed_stats">
{foreach from=$detailed name=DETAILED_STATS key=stat item=value}
<li><div class="left">{$stat}</div><div class="right">{if $stat == "start_time"}{$value|date_format:"%c"}{else}{$value}{/if}</div></li>
{/foreach}
</ul>
<br/>
{/if}

{if !empty($log_msg)}
<h2>Log Messages</h2>
<ul class="cameralog">
{foreach from=$log_msg name=LOGLIST item=msg}
<li class="lvl{$msg.lvl}">[{$msg.time|date_format:"%T"}] {$msg.msg|escape}</li>
{/foreach}
</ul>
{/if}

<h2>Cameras</h2>
<ul>
<li><a href="stats.php">System</a></li>
{foreach from=$camera_list name=CAMERALST item=cam key=cam_id}
<li><a href="stats.php?camera={$cam_id}">{$cam['name']|escape}{if $cam['active'] == 0} (disabled){/if}{if !empty($cam_status[$cam_id])} [{$cam_status[$cam_id]|escape}]{/if}</a></li>
{/foreach}
</ul>
{include file="footer.tpl"}