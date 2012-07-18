--TEST--
MongoMinKey insertion and sorting
--SKIPIF--
<?php require dirname(__FILE__) . "/skipif.inc";?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection(dbname(), 'mongominkey');
$coll->drop();

$coll->insert(array('x' => 1));
$coll->insert(array('x' => null));
$coll->insert(array('x' => new MongoDate()));
$coll->insert(array('x' => new MongoMinKey()));
$coll->insert(array('x' => 1.1));
$coll->insert(array('x' => false));

$cursor = $coll->find()->sort(array('x' => 1));

foreach ($cursor as $result) {
    if (is_object($result['x'])) {
        echo get_class($result['x']) . "\n";
    } else {
        echo json_encode($result['x']) . "\n";
    }
}
?>
--EXPECT--
MongoMinKey
null
1
1.1
false
MongoDate
