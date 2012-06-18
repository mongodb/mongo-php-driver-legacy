--TEST--
Endless loop on non-existing file
--FILE--
<?php
$m = new Mongo("mongodb://tmp:tmp@primary:27001/phpunit", array("replicaSet" => "foobar"));
$db = $m->selectDB("phpunit");
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

