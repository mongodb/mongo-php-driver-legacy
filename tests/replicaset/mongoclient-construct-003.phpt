--TEST--
MongoClient::__construct(): Connecting to replicaset member in a standalone mode (3)
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();
try {
    $mc = new MongoClient($rs["dsn"], array("readPreference" => MongoClient::RP_PRIMARY_PREFERRED));
    echo "ok\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
}

?>
--EXPECTF--
ok

