--TEST--
Test for PHP-361: Mongo::getHosts() segfaults when not connecting to a replica set
--FILE--
<?php
$m = new Mongo();
var_dump( $m->getHosts() );
?>
--EXPECT--
array(1) {
  ["localhost:27017"]=>
  array(2) {
    ["host"]=>
    string(9) "localhost"
    ["port"]=>
    int(27017)
  }
}
