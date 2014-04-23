--TEST--
Test for PHP-266: segfault when connection string refers to unknown replica set member.
--SKIPIF--
<?php require_once "tests/utils/replicaset.inc"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = hostname();
$port = port();
$ip   = gethostbyname($host);

$host2  = hostname("STANDALONE");
$port2 = port("STANDALONE");
$ip2   = gethostbyname($host2);

$m = new MongoClient("$ip:$port,$ip2:$port2", array("replicaSet" => true));
$coll = $m->selectCollection("phpunit","bug00266");
try {
    $coll->getIndexInfo();
    echo "I'm alive\n";
} catch(MongoCursorException $e) {
    var_dump($e->getMessage());
}
?>
--EXPECT--
I'm alive

