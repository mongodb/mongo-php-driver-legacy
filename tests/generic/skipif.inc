<?php
require __DIR__ . "/../others/skipif.inc";
if (!isset($_ENV["MONGO_SERVER"])) {
    die("skip Needs server context");
}

require __DIR__ ."/../utils.inc";
try {
    $m = null;
    $m = mongo();
} catch(Exception $e) {
    die("skip " . $e->getMessage());
}
if (!$m) {
    $cfg = __DIR__ . "/../mongo-test-cfg.inc";
    die("skip Did you remember to configure " . realpath($cfg) . " based on your environment?");
}

