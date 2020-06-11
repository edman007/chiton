{config_load file="main.conf" section="setup"}
{include file="header.tpl" title=Home}

<form method="post" action="settings.php">

{foreach from=$cfg_options key=opt_key item=opt name=CFGOPT}
Parameter: <input type="text" name="name[{$opt_key}]" value="{$opt['name']}"/>
Value: <input type="text" name="value[{$opt_key}]" value="{$opt['value']}"/>
Camera: <input type="text" name="camera[{$opt_key}]" value="{if $opt['camera'] == -1}ALL{else}{$opt['camera']}{/if}"/>
Delete <input type="checkbox" value="1" name="delete[{$opt_key}]"/>
<br />
{/foreach}
Parameter: <input type="text" name="name[{$smarty.foreach.CFGOPT.total}]" value=""/>
Value: <input type="text" name="value[{$smarty.foreach.CFGOPT.total}]" value=""/>
Camera: <input type="text" name="camera[{$smarty.foreach.CFGOPT.total}]" value=""/>
<br />

<input type="submit" name="Update"/>
</form>


{include file="footer.tpl"}