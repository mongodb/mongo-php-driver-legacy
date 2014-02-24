--TEST--
MongoWriteBatch::__construct() requires a valid operation type
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php if ( ! class_exists('MongoWriteBatch')) { exit('skip This test requires MongoWriteBatch classes'); } ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$mongo = mongo_standalone();
$collection = $mongo->selectCollection(dbname(), collname(__FILE__));

class MongoCustomWriteBatch extends MongoWriteBatch {
    public function __construct(MongoCollection $collection, $type) {
        parent::__construct($collection, $type);
    }
}

try {
    new MongoCustomWriteBatch($collection, -1);
    printf("MongoWriteBatch::__construct() accepted invalid type!\n");
} catch (MongoException $e) {
    printf("%d: %s", $e->getCode(), $e->getMessage());
}

?>
--EXPECT--
1: Invalid batch type specified: -1
