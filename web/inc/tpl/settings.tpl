{config_load file="main.conf" section="setup"}
{include file="header.tpl" title=Home}

<form method="post" action="settings.php">
<ul class="settings">
{foreach from=$set_keys item=set_key name=CFGOPT}
<li>
 <div class="setting_parameter">{$cfg_values[$set_key]['short_desc']|escape}
<input type="hidden" name="name[{$smarty.foreach.CFGOPT.index}]" value="{$set_key}"/>
</div>
 <div class="setting_value
 {if $cfg_values[$set_key]['set'] == 'default'}setting_default{/if}
 {if $cfg_values[$set_key]['set'] == 'system'}setting_system{/if}
 {if $cfg_values[$set_key]['set'] == 'camera'}setting_camera{/if}
">
 {if $set_key == 'timezone'}
 {*Special restriction for timezones*}
<select name="value[{$smarty.foreach.CFGOPT.index}]">
{foreach from=$timezones name=TIMEZN item=tz}
<option value="{$tz|escape}" {if $tz == $cfg_values[$set_key]['value']}selected{/if}>{$tz|escape}</option>
{/foreach}
</select>
{else}
<input type="text" name="value[{$smarty.foreach.CFGOPT.index}]" value="{$cfg_values[$set_key]['value']|escape}" {if $cfg_values[$set_key]['read_only']}readonly{/if}/>
{/if}</div>
<div class="setting_delete"><input type="hidden" name="camera[{$smarty.foreach.CFGOPT.index}]" value="{$camera_id}"/>
Delete <input type="checkbox" value="1" name="delete[{$smarty.foreach.CFGOPT.index}]"/></div>
</li>
{/foreach}
<li><div class="setting_parameter"><select name="name[{$smarty.foreach.CFGOPT.total}]">
{foreach from=$unset_keys item=unset_key name=UNSETOPT}
<option value="{$cfg_values[$unset_key]['key']}">{$cfg_values[$unset_key]['short_desc']|escape}</option>
{/foreach}
</select></div>
<div class="setting_value"><input type="text" name="value[{$smarty.foreach.CFGOPT.total}]" value=""/><input type="hidden" name="camera[{$smarty.foreach.CFGOPT.total}]" value="{$camera_id}"/></div>
 </li>
</ul>
<input type="submit" name="Update"/>
</form>

{include file="footer.tpl"}