--TEST--
Test for Mongo->getConnections()
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
var_dump($m->getConnections());
?>
--EXPECTF--
array(1) {
  [0]=>
  array(3) {
    ["hash"]=>
    string(%d) "%s:%d;-;.;%d"
    ["server"]=>
    array(4) {
      ["host"]=>
      string(%d) "%s"
      ["port"]=>
      int(%d)
      ["pid"]=>
      int(%d)
      ["version"]=>
      array(4) {
        ["major"]=>
        int(%d)
        ["minor"]=>
        int(%d)
        ["mini"]=>
        int(%d)
        ["build"]=>
        int(%i)
      }
    }
    ["connection"]=>
    array(8) {
      ["last_ping"]=>
      int(%d)
      ["last_ismaster"]=>
      int(%d)
      ["ping_ms"]=>
      int(%d)
      ["connection_type"]=>
      int(1)
      ["connection_type_desc"]=>
      string(10) "STANDALONE"
      ["max_bson_size"]=>
      int(%d)
      ["tag_count"]=>
      int(0)
      ["tags"]=>
      array(0) {
      }
    }
  }
}
