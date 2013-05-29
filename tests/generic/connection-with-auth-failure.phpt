--TEST--
Connection strings: unsuccesfull authentication
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
try {
    $host = hostname();
    $port = standalone_port();
    $user = "wrong username";
    $pass = "failed password";
    $db   = dbname();

    $m = new MongoClient(sprintf("mongodb://%s:%s@%s:%d/%s", $user, $pass, $host, $port, $db));
    echo "Have mongo client object\n";
    $m->$db->collection->findOne();
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}
?>
--EXPECTF--
Failed to connect to: %s:%d: Authentication failed on database '%s' with username '%S': auth%S
