--TEST--
MongoClient::__construct()#001 Connecting to replicaset
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$rs = MongoShellServer::getReplicasetInfo();
try {
    $mc = new MongoClient($rs["dsn"], array("replicaSet" => $rs["rsname"]));
    echo "ok\n";
} catch(Exception $e) {
    var_dump(get_class($e), $e->getMessage());
}

?>
--EXPECTF--
ok

