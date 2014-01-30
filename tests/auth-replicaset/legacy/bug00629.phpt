--TEST--
Test for PHP-629: Missing support for passing the username/password in the $options array
--SKIPIF--
<?php require_once "tests/utils/auth-replicaset.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$s = new MongoShellServer;
$cfg = $s->getReplicaSetConfig(true);
$creds = $s->getCredentials();

$opts = array(
    "db" => "admin",
    "username" => $creds["admin"]->username,
    "password" => $creds["admin"]->password,
    "replicaSet" => $cfg["rsname"],
);

$m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));
$response = $m->admin->command(array('buildInfo' => 1));
dump_these_keys($response, array('version', 'ok'));
?>
--EXPECTF--
array(2) {
  ["version"]=>
  string(%d) "%s"
  ["ok"]=>
  float(1)
}
