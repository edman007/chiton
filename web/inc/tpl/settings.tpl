{config_load file="main.conf" section="setup"}
{include file="header.tpl" title=Home}
<script>var cfgDB = "{$cfg_json|escape:'javascript'}";</script>
{if $camera_id < 0}
<h2>System Settings</h2>
{else}
<h2>Camera Settings</h2>
{/if}
<form method="post" action="settings.php?camera={$camera_id}">
<ul class="settings">
{foreach from=$set_keys item=set_key name=CFGOPT}
{if $camera_id < 0 || !$cfg_values[$set_key]['read_only']}
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
    <div class="setting_delete">
      <input type="hidden" name="camera[{$smarty.foreach.CFGOPT.index}]" value="{$camera_id}"/>
      {if !$cfg_values[$set_key]['read_only'] && !$cfg_values[$set_key]['required']}{* Read Only and required cannot be deleted *}
         {if $camera_id < 0}Use Default Value{else}Use System Value{/if}
         <input type="checkbox" value="1" name="delete[{$smarty.foreach.CFGOPT.index}]"
           {if ($camera_id < 0 && $cfg_values[$set_key]['set'] == 'default') || ($camera_id >= 0 && $cfg_values[$set_key]['set'] != 'camera')} checked{/if}/>
      {elseif $cfg_values[$set_key]['read_only']}
      Read Only
      {else}
      Required
      {/if}
    </div>
   </li>
{/if}
{/foreach}
<li><div class="setting_parameter"><select name="name[{$smarty.foreach.CFGOPT.total}]">
<option value="">Add New Option</option>
{foreach from=$unset_keys item=unset_key name=UNSETOPT}
{if !$cfg_values[$unset_key]['read_only']}
    <option value="{$cfg_values[$unset_key]['key']}">{$cfg_values[$unset_key]['short_desc']|escape}</option>
{/if}
{/foreach}
</select></div>
<div class="setting_value"><input type="text" name="value[{$smarty.foreach.CFGOPT.total}]" value=""/><input type="hidden" name="camera[{$smarty.foreach.CFGOPT.total}]" value="{$camera_id}"/></div>
 </li>
</ul>
<input type="submit" name="Update"/>
</form>
<a href="settings.php">System Settings</a>
<h2>Cameras</h2>
<ul>
{foreach from=$camera_list name=CAMERALST item=cam key=cam_id}
{if $cam_id != $camera_id}
<li><a href="settings.php?camera={$cam_id}">{$cam['name']|escape}{if $cam['active'] == 0} (disabled){/if}</a></li>
{/if}
{/foreach}
</ul>
{include file="footer.tpl"}