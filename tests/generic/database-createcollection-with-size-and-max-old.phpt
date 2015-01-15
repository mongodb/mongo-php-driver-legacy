--TEST--
Database: Create collection with max size and items (old)
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

var_dump(findCollection($d, 'createcol1'));

// create
$c = $d->createCollection('createcol1', true, 1000, 5);
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
--EXPECTF--
NULL

%s: MongoDB::createCollection(): This method now accepts arguments as an options array instead of the three optional arguments for capped, size and max elements in %sdatabase-createcollection-with-size-and-max-old.php on line 14
string(10) "createcol1"
int(5)
int(6)
int(7)
int(8)
int(9)
int(5)
