--TEST--
MongoDate micro/milliseconds discrepancy (with write)
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
$mongo = mongo_standalone();
$coll = $mongo->selectCollection(dbname(), 'mongodate');
$coll->drop();

$times = array(
	array( 0, 0 ), // epoch
	array( 0, 81234 ),
	array( 0, 801234 ),
	array( 0, 8001234 ),

	array( -1000, 81234 ),
	array( -1000, 801234 ),
	array( -1000, 8001234 ),

	array( -1000, -81234 ),
	array( -1000, -801234 ),
	array( -1000, -8001234 ),

	array( 1000, 81234 ),
	array( 1000, 801234 ),
	array( 1000, 8001234 ),
);

foreach ( $times as $time )
{
	list( $sec, $usec ) = $time;

	echo $sec, ', ', $usec, "\n";

	$a = new MongoDate( $sec, $usec );
	$obj = array( 'date' => $a );

	var_dump( $a );
	echo $a, "\n";
	$coll->insert( $obj );

	$encdec = $coll->findOne();
	var_dump( $encdec['date'] );
	echo $encdec['date'], "\n";
	echo "\n";

	$coll->remove();
}
?>
--EXPECTF--
0, 0
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(0)
}
0.00000000 0
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(0)
}
0.00000000 0

0, 81234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(81234)
}
0.08123400 0
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(81000)
}
0.08100000 0

0, 801234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(801234)
}
0.80123400 0
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(801000)
}
0.80100000 0

0, 8001234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(0)
  ["usec"]=>
  int(8001234)
}
8.00123400 0
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(8)
  ["usec"]=>
  int(1000)
}
0.00100000 8

-1000, 81234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(81234)
}
0.08123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(81000)
}
0.08100000 -1000

-1000, 801234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(801234)
}
0.80123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(801000)
}
0.80100000 -1000

-1000, 8001234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(8001234)
}
8.00123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-992)
  ["usec"]=>
  int(1000)
}
0.00100000 -992

-1000, -81234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(-81234)
}
-0.08123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1001)
  ["usec"]=>
  int(919000)
}
0.91900000 -1001

-1000, -801234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(-801234)
}
-0.80123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1001)
  ["usec"]=>
  int(199000)
}
0.19900000 -1001

-1000, -8001234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1000)
  ["usec"]=>
  int(-8001234)
}
-8.00123400 -1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(-1009)
  ["usec"]=>
  int(999000)
}
0.99900000 -1009

1000, 81234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1000)
  ["usec"]=>
  int(81234)
}
0.08123400 1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1000)
  ["usec"]=>
  int(81000)
}
0.08100000 1000

1000, 801234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1000)
  ["usec"]=>
  int(801234)
}
0.80123400 1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1000)
  ["usec"]=>
  int(801000)
}
0.80100000 1000

1000, 8001234
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1000)
  ["usec"]=>
  int(8001234)
}
8.00123400 1000
object(MongoDate)#%d (%d) {
  ["sec"]=>
  int(1008)
  ["usec"]=>
  int(1000)
}
0.00100000 1008
