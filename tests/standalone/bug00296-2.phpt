--TEST--
Test for PHP-296: MongoCollection->update() doesn't check option types
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php require_once "tests/utils/standalone.inc" ?>
<?php if (version_compare(PHP_VERSION, "5.3.0", "lt")) { exit("skip doesn't work on 5.2"); }?>
--FILE--
<?php
require_once "tests/utils/server.inc";

require_once "tests/utils/stream-notifications.inc";

$mn = new MongoNotifications;

$host = MongoShellServer::getStandaloneInfo();

function log_update($server, $old, $newobj, $flags, $insertopts) {
    echo __METHOD__, "\n";

    var_dump($flags, $insertopts);
}

function log_cmd_update($server, $write_options, $update_arguments, $protocol_options)
{
    $pretend = $pretend2 = array();
    $flags = 0;

    if ($update_arguments["upsert"]) {
        $pretend["upsert"] = true;
        $flags += 1;
    }
    if ($update_arguments["multi"]) {
        $pretend["multiple"] = true;
        $flags += 2;
    }

    $pretend2 = array(
        "namespace" => str_replace('$cmd', 'col', $protocol_options["namespace"]),
        "flags" => $flags
    );
    return log_update(null, null, null, $pretend, $pretend2);
}

$ctx = stream_context_create(
    array(
        "mongodb" => array(
            "log_update" => "log_update",
            "log_cmd_update" => "log_cmd_update",
        )),
    array(
        "notification" => array($mn, "update")
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
  object(stdClass)#%d (0) {
  }
  ["multiple"]=>
  object(stdClass)#%d (0) {
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
