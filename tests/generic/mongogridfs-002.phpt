--TEST--
MongoGridFS constructor with custom prefix
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = new MongoGridFS($db, 'foo');
printf("%s\n", $gridfs);
printf("%s\n", $gridfs->chunks);
--EXPECTF--
%s.foo.files
%s.foo.chunks
