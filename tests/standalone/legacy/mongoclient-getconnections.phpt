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
    array(12) {
      ["min_wire_version"]=>
      int(%d)
      ["max_wire_version"]=>
      int(%d)
      ["max_bson_size"]=>
      int(%d)
      ["max_message_size"]=>
      int(%d)
      ["max_write_batch_size"]=>
      int(%d)
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
      ["tag_count"]=>
      int(0)
      ["tags"]=>
      array(0) {
      }
    }
  }
}
