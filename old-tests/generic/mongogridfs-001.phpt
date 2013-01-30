--TEST--
MongoGridFS constructor with default prefix
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$db = $mongo->selectDB(dbname());

$gridfs = new MongoGridFS($db);
printf("%s\n", $gridfs);
printf("%s\n", $gridfs->chunks);
--EXPECTF--
%s.fs.files
%s.fs.chunks
