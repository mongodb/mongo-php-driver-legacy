--TEST--
Test for PHP-629
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";

$options = array(
	'connect'	=> true,
	'timeout'	=> 5000,
	'replicaSet' => rsname(),
	'username'	=> username("admin"),
	'password'	=> password("admin"),
);

$mongoDbClient = new MongoClient('mongodb://' . hostname(), $options);

$database = $mongoDbClient->selectDB('admin');
$command = "db.version()";
$response = $database->execute($command);
var_dump($response);
?>
--EXPECTF--
array(2) {
  ["retval"]=>
  string(%s) "%s"
  ["ok"]=>
  float(1)
}

