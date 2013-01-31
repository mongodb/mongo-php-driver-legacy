--TEST--
Connection strings: with database name and port
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

mongo("", false);
mongo("phpunit", false);
mongo("bar/baz", false);
mongo("/", true);

if (isset($_ENV["MONGO_SERVER"]) && $_ENV["MONGO_SERVER"] == "REPLICASET")  {
    new mongo("$REPLICASET_PRIMARY:$REPLICASET_PRIMARY_PORT,$REPLICASET_SECONDARY:$REPLICASET_SECONDARY_PORT");
}
?>
--EXPECT--
