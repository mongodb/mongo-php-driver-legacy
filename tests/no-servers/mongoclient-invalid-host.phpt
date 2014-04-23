--TEST--
MongoClient should throw exception on unresolvable hostname
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.3", '>=')) echo "skip >= PHP 5.3 needed\n"; ?>
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php

try {
    $m = new MongoClient("invalid-host-name");
} catch(MongoConnectionException $e) {
    var_dump($e->getMessage());
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
string(%d) "Failed to connect to: invalid-host-name:27017: %s"
===DONE===
