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

    'connect'    => true,
    'timeout'    => 5000,
);
$m = new MongoClient($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));

$database = $m->selectDB('admin');
$command = "db.version()";
$response = $database->execute($command);
dump_these_keys($response, array('retval', 'ok'));
?>
--EXPECTF--
%s: MongoClient::__construct(): The 'timeout' option is deprecated. Please use 'connectTimeoutMS' instead in %s on line %d
array(2) {
  ["retval"]=>
  string(%d) "%s"
  ["ok"]=>
  float(1)
}

