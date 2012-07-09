--TEST--
MongoDB Standalone
--SKIPIF--
<?php # vim:ft=php
if (!extension_loaded('mongo')) print 'skip mongo not loaded';
?>
--REDIRECTTEST--
return array(
    'ENV'   => array(),
    'TESTS' => "tests/generic",
);

