{if empty($raw_events)}
<h3>Events</h3>
<ul class="events">{/if}
{foreach from=$events item=ev name=EVENT_LIST}
<li onclick="CameraState.getCam({$camera_id}).jumpRealTime({$ev.start_ts});" id="ev_id{$ev.id}_{$ev.start_ts}">
{if !empty($ev.img)}<img src="{$ev.img}" />{/if} <div class="ev_txt"><a href="#{$camera_id},s{$ev.start_ts}">{$ev.start_txt}</a> <br /> {$ev.source} <br/>Score: {$ev.score}</div>
</li>
{/foreach}
{if empty($raw_events)}
</ul>
{/if}
