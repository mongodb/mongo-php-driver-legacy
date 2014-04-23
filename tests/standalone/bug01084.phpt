--TEST--
Test for PHP-1084: collection->update($query, $document) crashes if $document is an object
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);
$db = $mc->selectDb(dbname());
$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


$document = array("some" => "document");
$collection->insert($document);
$collection->update($document["_id"], array("data"));
$collection->remove($document["_id"]);
echo "I am alive\n";
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
I am alive
===DONE===

