--TEST--
Test for Mongo->getConnections()
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$m = mongo();
var_dump($m->getConnections());
?>
--EXPECTF--
array(1) {
	[0]=>
	array(3) {
		["hash"]=>
		string(%d) "%s:%d;-;X;%d"
		["server"]=>
		array(3) {
			["host"]=>
			string(%d) "%s"
			["port"]=>
			int(%d)
			["pid"]=>
			int(%d)
		}
		["connection"]=>
		array(8) {
			["last_ping"]=>
			int(%d)
			["last_ismaster"]=>
			int(0)
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
