--TEST--
Test for PHP-827: Segfault on connect when database name starts with an X
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$s = new MongoShellServer;
$host = $s->getStandaloneConfig(true);
$creds = $s->getCredentials();

$opts = array(
    "db" => "Xanadu",
    "username" => $creds["user"]->username,
    "password" => $creds["user"]->password,
);
try {
	$m = new MongoClient($host, $opts);
} catch (MongoConnectionException $e) {
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
echo "DONE\n";
?>
--EXPECTF--
71
Failed to connect to: %s:%d:%sAuthentication failed%s
DONE
