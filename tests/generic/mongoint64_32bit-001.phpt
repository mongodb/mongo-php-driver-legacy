--TEST--
MongoInt64 constructed with 32-bit integer
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc" ?>
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
--FILE--
<?php
require_once dirname(__FILE__) ."/../utils.inc";
$mongo = mongo();
$coll = $mongo->selectCollection('phpunit', 'mongoint64');
$coll->drop();

$coll->insert(array('int64' => new MongoInt64(1234567890)));

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', false);
$result = $coll->findOne();
var_dump($result['int64']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', false);
try {
	$coll->findOne();
} catch (Exception $e) {
	printf("%s: %s\n", get_class($e), $e->getMessage());
}

ini_set('mongo.native_long', false);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['int64']), $result['int64']);

ini_set('mongo.native_long', true);
ini_set('mongo.long_as_object', true);
$result = $coll->findOne();
printf("%s(%s)\n", get_class($result['int64']), $result['int64']);
?>
--EXPECT--
float(1234567890)
MongoCursorException: Can not natively represent the long 1234567890 on this platform
MongoInt64(1234567890)
MongoInt64(1234567890)
