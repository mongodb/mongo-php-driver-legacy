--TEST--
Test for PHP-296: MongoCollection->update() doesn't check option types
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

function log_update($server, $old, $newobj, $flags, $insertopts) {
    echo __METHOD__, "\n";

    var_dump($flags, $insertopts);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_update" => "log_update",
        )
    )
);

$mc = new MongoClient($host, array(), array("context" => $ctx));
$opts = array("upsert" => new stdclass, "multiple" => new stdclass);
$mc->test->col->update(array(array("doc" => 1)), array('$set' => array("doc" => 2)), $opts);
var_dump($opts);
$opts = array("upsert" => new stdclass);
$mc->test->col->update(array(array("doc" => 1)), array('$set' => array("doc" => 2)), $opts);
$opts = array("multiple" => new stdclass);
$mc->test->col->update(array(array("doc" => 1)), array('$set' => array("doc" => 2)), $opts);
?>
--EXPECTF--
log_update
array(2) {
  ["upsert"]=>
  bool(true)
  ["multiple"]=>
  bool(true)
}
array(2) {
  ["namespace"]=>
  string(8) "test.col"
  ["flags"]=>
  int(3)
}
array(2) {
  ["upsert"]=>
  object(stdClass)#2 (0) {
  }
  ["multiple"]=>
  object(stdClass)#3 (0) {
  }
}
log_update
array(1) {
  ["upsert"]=>
  bool(true)
}
array(2) {
  ["namespace"]=>
  string(8) "test.col"
  ["flags"]=>
  int(1)
}
log_update
array(1) {
  ["multiple"]=>
  bool(true)
}
array(2) {
  ["namespace"]=>
  string(8) "test.col"
  ["flags"]=>
  int(2)
}
