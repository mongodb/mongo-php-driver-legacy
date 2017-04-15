--TEST--
Test for PHP-447: Inconsistent error for unsupported database commands on mongod and mongos
--SKIPIF--
<?php require_once "tests/utils/mongos.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_query($server, $query, $cursor_options) {
    var_dump($query);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_query" => "log_query",
        )
    )
);

$cfg = MongoShellServer::getShardInfo();
$mc = new MongoClient($cfg[0], array("w" => 2, "readPreference" => MongoClient::RP_NEAREST), array("context" => $ctx));

$db = $mc->selectDB(dbname());
$retval = $db->command(array());
var_dump($retval);

?>
--EXPECTF--
array(2) {
  ["$query"]=>
  object(stdClass)#%d (0) {
  }
  ["$readPreference"]=>
  array(1) {
    ["mode"]=>
    string(7) "nearest"
  }
}
array(2) {
  ["ok"]=>
  float(0)
  ["errmsg"]=>
  string(%d) "no such cmd:%s"
}
