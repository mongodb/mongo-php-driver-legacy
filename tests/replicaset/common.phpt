--TEST--
MongoDB ReplicaSet
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('mongo')) print 'skip mongo not loaded';
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "REPLICASET"),
    'TESTS' => "tests/generic",
);

