--TEST--
Mongo::__construct() and INI options
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc"; ?>"
<?php
if (ini_get("mongo.default_host") == hostname() && ini_get("mongo.default_port") == port()) {
    exit("SKIP This test is meaningless when connecting to localhost:27017");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$host = hostname();
$port = port();
ini_set("mongo.default_host", $host);
ini_set("mongo.default_port", $port);

$m = new Mongo;
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

