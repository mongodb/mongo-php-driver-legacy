--TEST--
MongoDB Standalone via mongobridge
--SKIPIF--
<?php
// Force standalone mode
$_ENV["MONGO_SERVER"] = "BRIDGE_STANDALONE";
require __DIR__ ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "BRIDGE_STANDALONE"),
    'TESTS' => "tests/bridge",
);

