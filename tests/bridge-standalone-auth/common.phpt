--TEST--
MongoDB Standalone Auth via mongobridge
--SKIPIF--
<?php
// Force standalone auth mode
$_ENV["MONGO_SERVER"] = "BRIDGE_STANDALONE_AUTH";
require dirname(__FILE__) ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "BRIDGE_STANDALONE_AUTH"),
    'TESTS' => "tests/bridge",
);

