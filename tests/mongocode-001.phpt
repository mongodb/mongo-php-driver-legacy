--TEST--
MongoCode insertion with optional scope
--FILE--
<?php
$mongo = new Mongo('mongodb://localhost');
$coll = $mongo->selectCollection('test', 'mongocode');
$coll->drop();

$codeStr = 'return (x < 5);';

$coll->insert(array('_id' => 1, 'code' => new MongoCode($codeStr)));
$result = $coll->findOne(array('_id' => 1));
echo $result['code']->code . "\n";
echo json_encode($result['code']->scope) . "\n";

$coll->insert(array('_id' => 2, 'code' => new MongoCode($codeStr, array('x' => 2))));
$result = $coll->findOne(array('_id' => 2));
echo $result['code']->code . "\n";
echo json_encode($result['code']->scope) . "\n";
?>
--EXPECT--
return (x < 5);
[]
return (x < 5);
{"x":2}
