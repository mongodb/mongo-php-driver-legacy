--TEST--
Test for PHP-574: Problems with auth-switch and wrong credentials
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$s = new MongoShellServer;
$cfg = $s->getReplicasetConfig(true);
$creds = $s->getCredentials();

$opts = array(
    "db" => "test",
    "username" => $creds["user"]->username,
    "password" => $creds["user"]->password,
);
$m = new MongoClient($cfg["dsn"], $opts);
$db = $m->test2;
$db->authenticate('user2', 'user2' );
$collection = $db->collection;
try
{
	$collection->findOne();
}
catch ( Exception $e )
{
	echo $e->getMessage(), "\n";
}
echo "DONE\n";
?>
--EXPECTF--
Deprecated: Function MongoDB::authenticate() is deprecated in %sbug00574.php on line %d
anden.local:30303: unauthorized for query on test2.collection
DONE
