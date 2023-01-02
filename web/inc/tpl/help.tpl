{config_load file="main.conf" section="help"}
{include file="header.tpl" title=Help}
<h1>Chiton Config Values</h1>
More information is provided in the man page, see chiton.cfg(5)
{foreach from=$cfg_help item=cfg_txt name=cfg_loop}
{if isset($cfg_txt['priority_name'])}
<h1>{$cfg_txt['priority_name']} Settings</h1>
{/if}
<h2 id="{$cfg_txt['key']}">{$cfg_txt['key']}</h2>
<pre class="cfg_name">Display Name: {$cfg_txt['name']}</pre>
<pre class="cfg_default">Default Value: &quot;<span class="cfg_def_val">{$cfg_txt['default']}</span>&quot;</pre>
<pre class="cfg_short_desc">
Description: {$cfg_txt['short']}
</pre>
{/foreach}
{include file="footer.tpl"}