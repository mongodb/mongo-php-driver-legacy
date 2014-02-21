--TEST--
Test for PHP-296: MongoCollection->remove() doesn't check option types
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$host = MongoShellServer::getStandaloneInfo();

function log_delete($server, $criteria, $flags, $insertopts) {
    echo __METHOD__, "\n";

    var_dump($flags, $insertopts);
}

function log_cmd_delete($server, $write_options, $update_arguments, $protocol_options) {
    $pretend = $pretend2 = array();
    $flags = 0;

    if ($update_arguments["limit"]) {
        $pretend["justOne"] = (bool)$update_arguments["limit"];
        $flags = $update_arguments["limit"] ? 1 : 0;
    }
    $pretend2 = array(
        "namespace" => str_replace('$cmd', 'col', $protocol_options["namespace"]),
        "flags" => $flags
    );

    return log_delete(null, null, $pretend, $pretend2);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_delete" => "log_delete",
            "log_cmd_delete" => "log_cmd_delete",
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
  object(stdClass)#%d (0) {
  }
}
