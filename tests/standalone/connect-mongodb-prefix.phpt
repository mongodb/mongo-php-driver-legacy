--TEST--
Connection strings: Prefixed with mongodb://
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require_once dirname(__FILE__) . "/../utils.inc";
if ($STANDALONE_PORT != "27017") {
    die("skip This tests expects a server running on the default port");
}
?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

// The connect param will always be true in RS so we only test this standalone

$a = new Mongo("mongodb://$STANDALONE_HOSTNAME");
var_dump($a->connected);
$b = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
var_dump($b->connected);
$c = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT", array("connect" => false));
var_dump($c->connected);
--EXPECTF--
bool(true)
bool(true)

Deprecated: Mongo::__construct(): The 'connect' option is deprecated and will be removed in the future in %s on line %d
bool(false)
