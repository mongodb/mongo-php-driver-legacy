<?php
require dirname(__FILE__) . "/../others/skipif.inc";
require dirname(__FILE__) ."/../utils.inc";
$m = null;

try {
    $m = mongo();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}
if (!$m) {
    $cfg = dirname(__FILE__) . "/../mongo-test-cfg.inc";
    die("skip Did you remember to configure " . realpath($cfg) . " based on your environment?");
}

if (isset($needs)) {
    $serverversion = serverversion($m);
    if (version_compare($serverversion, $needs, "lt")) {
        die("skip This test requires MongoDB $needs, but you are running $serverversion");
    }
}

