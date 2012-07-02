--TEST--
MongoMinKey insertion and sorting
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongominkey');
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
