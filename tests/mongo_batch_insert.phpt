--TEST--
Test for issue PHP-233: support keep_going flag.
--FILE--
<?php
$db = new MongoDB(new Mongo(), "phpunit");
$object = $db->selectCollection('c');
$object->drop();

$doc1 = array(
	'_id' => new MongoId('4cb4ab6d7addf98506010001'),
	'id' => 1,
	'desc' => "ONE",
);
$doc2 = array(
	'_id' => new MongoId('4cb4ab6d7addf98506010002'),
	'id' => 2,
	'desc' => "TWO",
);
$doc3 = array(
	'_id' => new MongoId('4cb4ab6d7addf98506010002'),
	'id' => 3,
	'desc' => "THREE",
);
$doc4 = array(
	'_id' => new MongoId('4cb4ab6d7addf98506010004'),
	'id' => 4,
	'desc' => "FOUR",
);

$object->batchInsert(array($doc1, $doc2, $doc3, $doc4), array('continueOnError' => true));

$c = $object->find();
foreach ($c as $item) {
	var_dump($item);
}
?>
--EXPECTF--
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "4cb4ab6d7addf98506010001"
  }
  ["id"]=>
  int(1)
  ["desc"]=>
  string(3) "ONE"
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "4cb4ab6d7addf98506010002"
  }
  ["id"]=>
  int(2)
  ["desc"]=>
  string(3) "TWO"
}
array(3) {
  ["_id"]=>
  object(MongoId)#%d (1) {
    ["$id"]=>
    string(24) "4cb4ab6d7addf98506010004"
  }
  ["id"]=>
  int(4)
  ["desc"]=>
  string(4) "FOUR"
}
