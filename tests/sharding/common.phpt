--TEST--
MongoDB Sharding
--SKIPIF--
<?php
// Force sharding mode
$_ENV["MONGO_SERVER"] = "SHARDING";
require_once dirname(__FILE__) ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
	'ENV'	=> array("MONGO_SERVER" => "SHARDING"),
	'TESTS' => "tests/generic",
);

