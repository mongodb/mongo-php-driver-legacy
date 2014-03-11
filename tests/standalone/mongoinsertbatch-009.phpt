--TEST--
MongoWriteBatch: Ensure ctor is called
--SKIPIF--
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection("test", "insertbatch");
$collection->drop();

class MyInsertBatch extends MongoWriteBatch {
    public function __construct() {
    }
}

$batch = new MyInsertBatch($collection);
try {
    $batch->add(array("my" => "document"));
} catch(MongoException $e) {
    echo $e->getMessage(), "\n";
}

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
The MongoWriteBatch object has not been correctly initialized by its constructor
===DONE===
