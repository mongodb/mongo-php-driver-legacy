--TEST--
Test for PHP-805: Deprecate the "chunks" option in MongoGridFS::__construct.
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = new MongoGridFS($db, "foo", "bar");
$gridfs = $db->getGridFS("foo", "bar");
?>
--EXPECTF--
%s: MongoGridFS::__construct(): The 'chunks' argument is deprecated and ignored in %sbug00805.php on line %d

%s: MongoDB::getGridFS(): The 'chunks' argument is deprecated and ignored in %sbug00805.php on line %d
