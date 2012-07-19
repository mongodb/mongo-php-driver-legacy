--TEST--
Bug PHP-397 Endless loop on non-existing file
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
<?php exit("skip test requires authentication on a replicaset.."); ?>
--FILE--
<?php
$m = new Mongo("mongodb://user:user@primary.local:27001/phpunit-auth", array("replicaSet" => "foobar"));
$db = $m->selectDB("phpunit-unit");
$c = $db->selectCollection("example");
$c->setSlaveOkay(true);

$n = new MongoGridFs($db);
$b = new MongoGridFsFile($n, array("bar.txt", "length" => 42, "_id" => new MongoId("asdfasdf")));

try {
    $b->getBytes();
} catch(Exception $e) {
var_dump(get_class($e), $e->getMessage());
}
?>
===DONE===
--EXPECT--
===DONE===

