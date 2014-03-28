--TEST--
MongoWriteBatch: Allow custom batch instance
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

class MyInsertBatch extends MongoWriteBatch {
    public function __construct($client) {
        parent::__construct($client, MongoWriteBatch::COMMAND_INSERT);
    }
}

class MyUpdateBatch extends MongoWriteBatch {
    public function __construct($client) {
        parent::__construct($client, MongoWriteBatch::COMMAND_UPDATE);
    }
}

class MyDeleteBatch extends MongoWriteBatch {
    public function __construct($client) {
        parent::__construct($client, MongoWriteBatch::COMMAND_DELETE);
    }
}

class MyBrokenBatch extends MongoWriteBatch {
    public function __construct($client) {
        parent::__construct($client, 0);
    }
}

$host = MongoShellServer::getStandaloneInfo();

$mc = new MongoClient($host);

$collection = $mc->selectCollection(dbname(), collname(__FILE__));
$collection->drop();


$batch = new MyInsertBatch($collection);
$batch->add(array("my" => "document"));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
$batch->execute();
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($collection->findOne());

$batch = new MyUpdateBatch($collection);
$batch->add(array("q" => array("my" => "document"), "u" => array('$set' => array("got" => "updated"))));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
$batch->execute();
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($collection->findOne());

$batch = new MyDeleteBatch($collection);
$batch->add(array("q" => array("my" => "document"), "limit" => 1));
var_dump($batch->getItemCount(), $batch->getBatchInfo());
$batch->execute();
var_dump($batch->getItemCount(), $batch->getBatchInfo());
var_dump($collection->findOne());

try {
    $batch = new MyBrokenBatch($collection);
    echo "Failed\n";
} catch(Exception $e) {
    echo get_class($e), "\n";
    echo $e->getMessage(), "\n";
}
?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
int(1)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(143)
  }
}
int(0)
array(0) {
}
array(2) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["my"]=>
  string(8) "document"
}
int(1)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(168)
  }
}
int(0)
array(0) {
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "%s"
  }
  ["my"]=>
  string(8) "document"
  ["got"]=>
  string(7) "updated"
}
int(1)
array(1) {
  [0]=>
  array(2) {
    ["count"]=>
    int(1)
    ["size"]=>
    int(143)
  }
}
int(0)
array(0) {
}
NULL
MongoException
Invalid batch type specified: 0
===DONE===
