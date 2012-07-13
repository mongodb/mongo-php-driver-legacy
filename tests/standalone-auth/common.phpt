--TEST--
MongoDB Standalone with authentication
--SKIPIF--
<?php
// Force standalone mode
$_ENV["MONGO_SERVER"] = "STANDALONE_AUTH";
require __DIR__ ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "STANDALONE_AUTH"),
    'TESTS' => "tests/generic",
);

