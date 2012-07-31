<?php
require_once dirname(__FILE__) . "/../others/skipif.inc";
require_once dirname(__FILE__) ."/../utils.inc";
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

