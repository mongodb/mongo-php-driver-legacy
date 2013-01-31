--TEST--
Connection strings: Prefixed with mongodb://
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php
require_once "tests/utils/server.inc";
if ($STANDALONE_PORT != "27017") {
    die("skip This tests expects a server running on the default port");
}
?>
--FILE--
<?php
require_once "tests/utils/server.inc";

// The connect param will always be true in RS so we only test this standalone

$c = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT", array("connect" => false));
var_dump($c->connected);
$a = new Mongo("mongodb://$STANDALONE_HOSTNAME");
var_dump($a->connected);
$b = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
var_dump($b->connected);
--EXPECTF--
bool(false)
bool(true)
bool(true)
