--TEST--
Connection strings: Test multiple host names with/without port
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
<?php
require __DIR__ . "/../utils.inc";

if (isset($_ENV["MONGO_SERVER"]) && $_ENV["MONGO_SERVER"] == "REPLICASET") {
    $port = $REPLICASET_PRIMARY_PORT;
} else {
    $port = $STANDALONE_PORT;
}

if ($port != "27017") {
    die("skip this tests attempts to connect to the standard port");
}
?>
--FILE--
<?php
require __DIR__ . "/../utils.inc";

if (isset($_ENV["MONGO_SERVER"]) && $_ENV["MONGO_SERVER"] == "REPLICASET") {
    $hostname = $REPLICASET_PRIMARY;
    $ip = gethostbyname($hostname);
    $port = $REPLICASET_PRIMARY_PORT;
} else {
    $hostname = $STANDALONE_HOSTNAME;
    $ip = gethostbyname($hostname);
    $port = $STANDALONE_PORT;
}

$a = new Mongo("$hostname,$ip");
var_dump($a->connected);
$b = new Mongo("$hostname:$port,$ip:$port");
var_dump($b->connected);
?>
--EXPECT--
bool(true)
bool(true)
