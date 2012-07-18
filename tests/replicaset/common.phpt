--TEST--
MongoDB ReplicaSet
--SKIPIF--
<?php
// Force replicaset mode
$_ENV["MONGO_SERVER"] = "REPLICASET";
require dirname(__FILE__) ."/skipif.inc";
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "REPLICASET"),
    'TESTS' => "tests/generic",
);

