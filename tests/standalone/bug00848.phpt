--TEST--
Invalid read in $cursor->count()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$cursor = $mc
    ->selectDB(dbname())
    ->selectCollection("bug848")
    ->find()
    ->sort(array("_id" => 1));
$cursor->count();
foreach($cursor as $doc);
echo "ok\n";
?>
--EXPECTF--
ok

