<?php
define('EMONCMS_EXEC', 1);

chdir("/var/www/emoncms");
require "Lib/EmonLogger.php";
require "process_settings.php";
require "Modules/wifi/wifi.php";
$wifi = new Wifi();

$log = new EmonLogger(__FILE__);

if ($redis_enabled) {
    $redis = new Redis();
    if (!$redis->connect($redis_server['host'], $redis_server['port'])) {
        $log->error("Cannot connect to redis at ".$redis_server['host'].":".$redis_server['port']);  die('Check log\n');
    }
    if (!empty($redis_server['prefix'])) $redis->setOption(Redis::OPT_PREFIX, $redis_server['prefix']);
    if (!empty($redis_server['auth'])) {
        if (!$redis->auth($redis_server['auth'])) {
            $log->error("Cannot connect to redis at ".$redis_server['host'].", autentication failed"); die('Check log\n');
        }
    }
} else {
    $redis = false;
}

$redis->set("wifi/scan",json_encode($wifi->scan()));
