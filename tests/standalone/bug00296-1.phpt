--TEST--
Test for PHP-296: MongoCollection->batchInsert() doesn't check option types
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

function log_batchinsert($server, $dosc, $insertopts, $flags) {
    echo __METHOD__, "\n";

    var_dump($flags, $insertopts);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_batchinsert" => "log_batchinsert",
        )
    )
);

$mc = new MongoClient($host, array(), array("context" => $ctx));
$opts = array("continueOnError" => new stdclass);
$mc->test->col->batchInsert(array(array("doc" => 1)), $opts);
var_dump($opts);
?>
--EXPECTF--
log_batchinsert
array(1) {
  ["flags"]=>
  int(1)
}
array(1) {
  ["continueOnError"]=>
  bool(true)
}
array(1) {
  ["continueOnError"]=>
  object(stdClass)#2 (0) {
  }
}
