--TEST--
Database: Create collection with max size and items
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/collection-info.inc";

$a = mongo_standalone();
$d = $a->selectDb(dbname());

// cleanup
$d->selectCollection("createcol1")->drop();

$ns = $d->selectCollection('system.namespaces');
var_dump(findCollection($d, 'createcol1'));

// create
$c = $d->createCollection('createcol1', array('capped' => true, 'size' => 1000, 'max' => 5));
$retval = findCollection($d, 'createcol1');
var_dump($retval["name"]);

// test cap
for ($i = 0; $i < 10; $i++) {
    $c->insert(array('x' => $i), array("w" => true));
}
foreach($c->find() as $res) {
    var_dump($res["x"]);
}
var_dump($c->count());
?>
--EXPECT--
NULL
string(10) "createcol1"
int(5)
int(6)
int(7)
int(8)
int(9)
int(5)
