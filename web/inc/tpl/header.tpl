<!DOCTYPE html>
<html>
<head>
<title>{$title} - Chiton</title>
<link rel="stylesheet" href="static/default.css?v={$CHITON_VERSION}">
<script src="static/pinch-zoom-min.js?v={$CHITON_VERSION}" type="text/javascript"></script>
<script src="static/chiton.js?v={$CHITON_VERSION}" type="text/javascript"></script>
<script src="static/hls.light.min.js?v={$CHITON_VERSION}" type="text/javascript"></script>

</head>
<body onLoad="loadSite('{#pagetag#}');">
<ul class="sitemap">
<li><a href="index.php">Home</a></li>
<li><a href="settings.php">Settings</a></li>
<li><a href="help.php">Help</a></li>
<li><a href="license.php">License</a></li>
</ul>
{if !empty($system_msg)}
<ul class="statusmsg">
{foreach from=$system_msg item=msg name=SYSTEM_MSG_BANNER}
<li>{$msg|escape}</li>
{/foreach}
</ul>
{/if}