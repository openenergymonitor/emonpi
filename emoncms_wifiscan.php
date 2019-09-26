<?php
define('EMONCMS_EXEC', 1);

chdir("/var/www/emoncms");
require "Lib/EmonLogger.php";
require "process_settings.php";
require "Modules/wifi/wifi.php";
$wifi = new Wifi();

$log = new EmonLogger(__FILE__);

if ($settings['redis']['enabled']) {
    $redis = new Redis();
    $connected = $redis->connect($settings['redis']['host'], $settings['redis']['port']);
    if (!$connected) { echo "Can't connect to redis at ".$settings['redis']['host'].":".$settings['redis']['port']." , it may be that redis-server is not installed or started see readme for redis installation"; die; }
    if (!empty($settings['redis']['prefix'])) $redis->setOption(Redis::OPT_PREFIX, $settings['redis']['prefix']);
    if (!empty($settings['redis']['auth'])) {
        if (!$redis->auth($settings['redis']['auth'])) {
            echo "Can't connect to redis at ".$settings['redis']['host'].", autentication failed"; die;
        }
    }
    if (!empty($settings['redis']['dbnum'])) {
        $redis->select($settings['redis']['dbnum']);
    }
} else {
    $redis = false;
}

$redis->set("wifi/scan",json_encode($wifi->scan()));
