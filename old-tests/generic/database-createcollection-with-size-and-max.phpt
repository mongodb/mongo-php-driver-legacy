--TEST--
Database: Create collection with max size and items
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$a = mongo();
$d = $a->selectDb("phpunit");

// cleanup
$d->selectCollection("createcol1")->drop();

$ns = $d->selectCollection('system.namespaces');
var_dump($ns->findOne(array('name' => 'phpunit.createcol1')));

// create
$c = $d->createCollection('createcol1', true, 1000, 5);
$retval = $ns->findOne(array('name' => 'phpunit.createcol1'));
var_dump($retval["name"]);

// test cap
for ($i = 0; $i < 10; $i++) {
    $c->insert(array('x' => $i), array("safe" => true));
}
foreach($c->find() as $res) {
    var_dump($res["x"]);
}
var_dump($c->count());
?>
--EXPECT--
NULL
string(18) "phpunit.createcol1"
int(5)
int(6)
int(7)
int(8)
int(9)
int(5)
