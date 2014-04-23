--TEST--
Test for PHP-585: GridFS incorrectly reports w=1 as w=0
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = new_mongo_standalone();
var_dump(get_class($m));
$gridfs = $m->selectDb(dbname())->getGridFS();
var_dump($gridfs->w);

$m = mongo_standalone();
var_dump(get_class($m));
$gridfs = $m->selectDb(dbname())->getGridFS();
var_dump($gridfs->w);

$m = old_mongo_standalone();
var_dump(get_class($m));
$gridfs = $m->selectDb(dbname())->getGridFS();
var_dump($gridfs->w);
?>
--EXPECTF--
string(11) "MongoClient"
int(1)
string(11) "MongoClient"
int(1)

%s: %s(): The Mongo class is deprecated, please use the MongoClient class in %sserver.inc on line %d
string(5) "Mongo"
int(1)
