<?php
require __DIR__ . "/../others/skipif.inc";
require __DIR__ ."/../utils.inc";
$m = null;

try {
    $m = mongo();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}
if (!$m) {
    $cfg = __DIR__ . "/../mongo-test-cfg.inc";
    die("skip Did you remember to configure " . realpath($cfg) . " based on your environment?");
}

