--TEST--
MongoCollection::aggregateCursor() with invalid cursor option
--SKIPIF--
<?php $needs = "2.5.3"; require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();
$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();

for ($i = 0; $i < 10; $i++) {
    $collection->insert(array('article_id' => $i));
}

try {
    $cursor = $collection->aggregateCursor(
        array(array('$limit' => 2)),
        array('cursor' => 42)
    );
} catch (MongoCursorException $e) {
    printf("exception message: %s\n", $e->getMessage());
    printf("exception code: %d\n", $e->getCode());
}

?>
===DONE===
--EXPECT--
exception message: The cursor command's 'cursor' element is not an array
exception code: 32
===DONE===
