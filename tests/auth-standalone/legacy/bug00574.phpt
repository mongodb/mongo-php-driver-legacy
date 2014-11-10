--TEST--
Test for PHP-574: Problems with auth-switch and wrong credentials
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$s = new MongoShellServer;
$host = $s->getStandaloneConfig(true);
$creds = $s->getCredentials();

$opts = array(
    "db" => "test",
    "username" => $creds["user"]->username,
    "password" => $creds["user"]->password,
);
$m = new MongoClient($host, $opts);
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
%s: Function MongoDB::authenticate() is deprecated in %s on line %d

Warning: MongoDB::authenticate(): You can't authenticate an already authenticated connection. in %s on line %d
Failed to connect to: %s:%d:%sAuthentication failed on database 'test2'%s
DONE
