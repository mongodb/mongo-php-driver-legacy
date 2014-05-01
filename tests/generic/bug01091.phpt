--TEST--
Test for PHP-1091: Allow NumberLong values on 32-bit platforms if they fit in the 32-bit int range.
--SKIPIF--
<?php if (PHP_INT_SIZE !== 4) { die('skip Only for 32-bit platform'); } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--INI--
mongo.long_as_object=0
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

$m = new MongoClient($host);
$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

$numbers = array(
	new MongoInt64("123"),
	new MongoInt64("9876543210"),
	new MongoInt64("-123"),
	new MongoInt64("-9876543210"),
	new MongoInt64("2147483647"),
	new MongoInt64("2147483648"),
	new MongoInt64("2147483649"),
	new MongoInt64("-2147483647"),
	new MongoInt64("-2147483648"),
	new MongoInt64("-2147483649"),
);

foreach ( $numbers as $key => $item )
{
	$c->insert( array( '_id' => $key, 'n' => $item ) );
}

foreach ( $numbers as $key => $dummy )
{
	echo $key, ": ";
	try {
		$item = $c->findOne( array( '_id' => $key ) );
		var_dump( $item['n'] );
	} catch ( MongoCursorException $e ) {
		echo $e->getMessage(), "\n";
	}
}
?>
--EXPECT--
0: int(123)
1: Cannot natively represent the long 9876543210 on this platform
2: int(-123)
3: Cannot natively represent the long -9876543210 on this platform
4: int(2147483647)
5: Cannot natively represent the long 2147483648 on this platform
6: Cannot natively represent the long 2147483649 on this platform
7: int(-2147483647)
8: int(-2147483648)
9: Cannot natively represent the long -2147483649 on this platform
