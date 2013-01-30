--TEST--
Test for PHP-361: Mongo::getHosts() segfaults when not connecting to a replica set.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
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
