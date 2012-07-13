--TEST--
Test for PHP-361: Mongo::getHosts() segfaults when not connecting to a replica set
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";
$m = mongo();

$hosts = $m->getHosts();
if ($hosts && is_array($hosts)) {
    echo "ok\n";
}

$host = current($hosts);
echo $host["host"], ":", $host["port"], "\n";
?>
--EXPECTF--
ok
%s:%d
