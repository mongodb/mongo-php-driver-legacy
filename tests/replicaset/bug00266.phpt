--TEST--
Bug#00266 (segfault when connection string refers to unknown replica set member)
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
<?php if (!$STANDALONE_HOSTNAME) { exit("skip Needs a standalone server too"); } ?>
--FILE--
<?php require __DIR__ ."/cfg.inc"; ?>
<?php
$host = hostname();
$port = port();
$ip   = gethostbyname($host);

$host2  = hostname("STANDALONE");
$port2 = port("STANDALONE");
$ip2   = gethostbyname($host2);

$m = new Mongo("$ip:$port,$ip2:$port2", array("replicaSet" => true));
$coll = $m->selectCollection("phpunit","bug00266");
try {
    print_r($coll->getIndexInfo());
} catch(MongoCursorException $e) {
    var_dump($e->getMessage());
}
?>
--EXPECT--
string(25) "couldn't determine master"

