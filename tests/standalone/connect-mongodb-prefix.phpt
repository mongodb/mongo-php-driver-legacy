--TEST--
Connection strings: Prefixed with mongodb://
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
<?php
require dirname(__FILE__) . "/../utils.inc";
if ($STANDALONE_PORT != "27017") {
    die("skip This tests expects a server running on the default port");
}
?>
--FILE--
<?php
require dirname(__FILE__) . "/../utils.inc";

// The connect param will always be true in RS so we only test this standalone

$a = new Mongo("mongodb://$STANDALONE_HOSTNAME", false);
var_dump($a->connected);
$a = new Mongo("mongodb://$STANDALONE_HOSTNAME");
var_dump($a->connected);
$b = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT", false);
var_dump($b->connected);
$b = new Mongo("mongodb://$STANDALONE_HOSTNAME:$STANDALONE_PORT");
var_dump($b->connected);
--EXPECTF--
%s: Mongo::__construct(): Passing scalar values for the options parameter is deprecated and will be removed in the near future in %s on line %d
bool(false)
bool(true)

%s: Mongo::__construct(): Passing scalar values for the options parameter is deprecated and will be removed in the near future in %s on line %d
bool(false)
bool(true)
