--TEST--
Test for PHP-585: GridFS incorrectly reports w=1 as w=0
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = new_mongo();
var_dump(get_class($m));
$gridfs = $m->selectDb(dbname())->getGridFS();
var_dump($gridfs->w);


$m = mongo();
var_dump(get_class($m));
$gridfs = $m->selectDb(dbname())->getGridFS();

var_dump($gridfs->w);
?>
--EXPECTF--
string(11) "MongoClient"
int(1)
string(5) "Mongo"
int(1)

