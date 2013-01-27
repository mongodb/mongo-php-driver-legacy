--TEST--
MongoDB Standalone with authentication
--SKIPIF--
<?php
// Force standalone mode
$_ENV["MONGO_SERVER"] = "STANDALONE_AUTH";
require_once dirname(__FILE__) ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
	'ENV'	=> array("MONGO_SERVER" => "STANDALONE_AUTH"),
	'TESTS' => "tests/generic",
);

