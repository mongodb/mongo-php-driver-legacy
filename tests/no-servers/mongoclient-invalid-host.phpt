--TEST--
MongoClient should throw exception on unresolvable hostname
--SKIPIF--
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
