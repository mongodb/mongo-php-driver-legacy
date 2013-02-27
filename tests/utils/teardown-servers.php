<?php
$fp = fsockopen("localhost", 8000, $errno, $errstr, 2);
if (!$fp) {
    throw new Exception($errstr, $errno);
}
fwrite($fp, "Sorry Matt Damon, we're out of time\n");
fflush($fp);
echo stream_get_contents($fp);
fflush($fp);
fclose($fp);

