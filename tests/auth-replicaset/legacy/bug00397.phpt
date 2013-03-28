--TEST--
Test for PHP-397: Endless loop on non-existing file.
--SKIPIF--
<?php require_once "tests/utils/auth-replicaset.inc"; ?>
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
);
$m = new Mongo($cfg["dsn"], $opts+array("readPreference" => MongoClient::RP_SECONDARY_PREFERRED));

$db = $m->selectDB("phpunit-unit");
$c = $db->selectCollection("example");

$n = new MongoGridFs($db);
$b = new MongoGridFsFile($n, array("bar.txt", "length" => 42, "_id" => new MongoId()));

try {
    $b->getBytes();
} catch(Exception $e) {
var_dump(get_class($e), $e->getMessage());
}
?>
===DONE===
--EXPECT--
===DONE===

