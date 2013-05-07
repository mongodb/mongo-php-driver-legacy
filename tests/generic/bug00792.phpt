--TEST--
Test for PHP-792: Memory leak while reading an INT64 on a 32bit platform with native_long enabled.
--SKIPIF--
<?php if (4 !== PHP_INT_SIZE) { die('skip Only for 32-bit platform'); } ?>
<?php require "tests/utils/standalone.inc";?>
--INI--
report_memleaks=1
display_errors=1
mongo.native_long=1
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection(dbname(), 'bug00792');
$c->drop();

$i = new MongoInt64("8237468276182323423");
$c->insert( array( '_id' => 1, 'i' => $i ) );

try
{
	var_dump( $c->findOne( array( "_id" => 1 ) ) );
} catch(Exception $e)
{
	echo $e->getCode(), "\n";
	echo $e->getMessage(), "\n";
}
?>
--EXPECT--
23
Can not natively represent the long 8237468276182323423 on this platform
