--TEST--
MongoDB ReplicaSet with authentication
--SKIPIF--
<?php
// Force replicaset mode
$_ENV["MONGO_SERVER"] = "REPLICASET_AUTH";
require __DIR__ ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "REPLICASET_AUTH"),
    'TESTS' => "tests/generic",
);

