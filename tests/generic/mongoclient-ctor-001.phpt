--TEST--
Mongo::__construct() and INI options
--SKIPIF--
<?php require "tests/utils/standalone.inc"; ?>
<?php
if (ini_get("mongo.default_host") == hostname() && ini_get("mongo.default_port") == standalone_port()) {
    exit("SKIP This test is meaningless when connecting to localhost:27017");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = standalone_hostname();
$port = standalone_port();
ini_set("mongo.default_host", $host);
ini_set("mongo.default_port", $port);

$m = new MongoClient;
foreach($m->getHosts() as $s) {
    if ($s["host"] == $host) {
        echo "Connected to correct host\n";
    }
    if ($s["port"] == $port) {
        echo "Connected to correct port\n";
    }
}
echo "All done\n";
?>
--EXPECT--
Connected to correct host
Connected to correct port
All done

