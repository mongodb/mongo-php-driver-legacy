--TEST--
Test for PHP-1218: Add MongoDate->toDateTime to allow creation of DateTime object from MongoDate
--SKIPIF--
<?php if (!version_compare(phpversion(), "5.4", '<')) echo "skip < PHP 5.4 needed\n"; ?>
--INI--
date.timezone=UTC
--FILE--
<?php
$dates = array(
	array( 1410173049, 0 ),
	array( -123101231, 0 ),
);

foreach ( $dates as $date )
{
	$m = new MongoDate( $date[0], $date[1] );
	var_dump( $m->toDateTime() );
}
?>
--EXPECTF--
object(DateTime)#%d (3) {
  ["date"]=>
  string(19) "2014-09-08 10:44:09"
  ["timezone_type"]=>
  int(1)
  ["timezone"]=>
  string(6) "+00:00"
}
object(DateTime)#%d (3) {
  ["date"]=>
  string(19) "1966-02-06 05:12:49"
  ["timezone_type"]=>
  int(1)
  ["timezone"]=>
  string(6) "+00:00"
}
