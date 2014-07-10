<html>
<?php

// By Eric AMANN
// http://openenergymonitor.org/emon/node/5376
$nodeid = 0; // change "0" with the node id that will receive data
$apikey="XXXXXXXXXXXXXX"; // change "XXXXXXXXXXXXXX" with your APIKEY
$target="emoncms.org"; //or "localhost/emoncms"

$mem = preg_split('#\s+#',shell_exec("free -m | grep Me"));
$GPU_temp=round(0.001*shell_exec("cat /sys/class/thermal/thermal_zone0/temp"));
$CPU_temp=1*shell_exec('/opt/vc/bin/vcgencmd measure_temp | cut -d "=" -f2 | cut -d "." -f1');
$cmd='top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9.]*\)%* id.*/\1/" | awk \'{print 100 - $1}\'';
$CPU_usage=round(shell_exec($cmd));

$url = 'http://'.$target.'/input/post.json?json={mem_used:'.$mem[2].',mem_free:'.$mem[3].',mem_shared:'.$mem[4].',mem_buffers:'.$mem[5].',mem_cached:'.$mem[6].',GPU_temp:'.$GPU_temp.',CPU_temp:'.$CPU_temp.',CPU_usage:'.$CPU_usage.'}&node='.$nodeid.'&apikey='.$apikey ;

//echo $url."\n";

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL,$url);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
$contents = curl_exec ($ch);
curl_close ($ch);

?>
</html>


