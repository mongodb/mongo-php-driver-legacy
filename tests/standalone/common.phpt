--TEST--
MongoDB Standalone
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('mongo')) print 'skip mongo not loaded';
?>
--REDIRECTTEST--
return array(
    'ENV'   => array("MONGO_SERVER" => "STANDALONE"),
    'TESTS' => "tests/generic",
);

