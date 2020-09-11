{config_load file="main.conf" section="license"}
{include file="header.tpl" title=Home}

{if !empty($no_license)}
<h1>Check your configuration</h1>
We couldn't find the full license text, but a summary is below.
<pre class="license">
Chiton is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Chiton is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Chiton.  If not, see <https://www.gnu.org/licenses/>.

Copyright 2020 Ed Martin <edman007@edman007.com>
</pre>
{else}
<h1>The full license for Chiton is below</h1>
<pre class="license">{$license['chiton']}</pre>
{foreach from=$license['third_party'] item=license_txt name=license_loop}
<h2>Chiton includes {$license_txt['name']}</h2>
<pre class="license">
{$license_txt['text']}
</pre>
{/foreach}
{/if}
{include file="footer.tpl"}