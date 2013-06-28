--TEST--
Test for PHP-858: Crash when extending Mongo and MongoClient classes and not calling its constructor
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
class T extends Mongo {
    public function __construct() {}
}
class U extends MongoClient {
    public function __construct() {}
}
class V extends MongoDB {
    public function __construct($a, $b) {}
}
class X extends MongoCollection {
    public function __construct() {}
}
var_dump(new T);
var_dump(new U);
var_dump(new V(1,2));
var_dump(new X(1,2));
?>
--EXPECTF--
object(T)#%d (4) {
  ["connected"]=>
  bool(false)
  ["status"]=>
  NULL
  ["server%S:protected%S]=>
  NULL
  ["persistent%Sprotected%S]=>
  NULL
}
object(U)#%d (4) {
  ["connected"]=>
  bool(false)
  ["status"]=>
  NULL
  ["server%S:protected%S]=>
  NULL
  ["persistent%Sprotected%S]=>
  NULL
}
object(V)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(X)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}

