--TEST--
MongoClient::__construct(): Connecting to one mongos (1)
--SKIPIF--
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$cfg = MongoShellServer::getShardInfo();
try {
    $mc = new MongoClient($cfg[0]);
    echo "ok\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
}

?>
--EXPECTF--
ok

