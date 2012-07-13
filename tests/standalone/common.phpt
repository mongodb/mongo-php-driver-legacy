--TEST--
MongoDB Standalone
--SKIPIF--
<?php
// Force standalone mode
$_ENV["MONGO_SERVER"] = "STANDALONE";
require __DIR__ ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "STANDALONE"),
    'TESTS' => "tests/generic",
);

