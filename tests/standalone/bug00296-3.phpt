--TEST--
Test for PHP-296: MongoCollection->remove() doesn't check option types
--SKIPIF--
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

function log_delete($server, $criteria, $flags, $insertopts) {
    echo __METHOD__, "\n";

    var_dump($flags, $insertopts);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_delete" => "log_delete",
        )
    )
);

$mc = new MongoClient($host, array(), array("context" => $ctx));
$opts = array("justOne" => new stdclass);
$mc->test->col->remove(array(array("doc" => 1)), $opts);
var_dump($opts);
?>
--EXPECTF--
log_delete
array(1) {
  ["justOne"]=>
  bool(true)
}
array(2) {
  ["namespace"]=>
  string(8) "test.col"
  ["flags"]=>
  int(1)
}
array(1) {
  ["justOne"]=>
  object(stdClass)#2 (0) {
  }
}
