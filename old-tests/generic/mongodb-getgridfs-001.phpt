--TEST--
MongoDB::getGridFS() with default prefix
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$db = $mongo->selectDB(dbname());

$gridfs = $db->getGridFS();
printf("%s\n", $gridfs);
printf("%s\n", $gridfs->chunks);
--EXPECTF--
%s.fs.files
%s.fs.chunks
