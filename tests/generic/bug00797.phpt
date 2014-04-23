--TEST--
Test for PHP-797: Deprecate public properties
--SKIPIF--
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$mc = new MongoClient("..", array("connect" => false));
var_dump($mc->status, $mc->server, $mc->persistent, $mc->connected);

class MyMongoClient extends MongoClient {
    public function dump() {
        var_dump($this->status, $this->server, $this->persistent, $this->connected);
    }
}

$mc = new MyMongoClient("..", array("connect" => false));
$mc->dump();

?>
--EXPECTF--
%s: main(): The 'status' property is deprecated in %s on line %d

%s: main(): The 'connected' property is deprecated in %s on line %d
NULL
object(MongoDB)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
object(MongoDB)#%d (2) {
  ["w"]=>
  int(1)
  ["wtimeout"]=>
  int(10000)
}
bool(false)

%s: MyMongoClient::dump(): The 'status' property is deprecated in %s on line %d

%s: MyMongoClient::dump(): The 'server' property is deprecated in %s on line %d

%s: MyMongoClient::dump(): The 'persistent' property is deprecated in %s on line %d

%s: MyMongoClient::dump(): The 'connected' property is deprecated in %s on line %d
NULL
NULL
NULL
bool(false)
