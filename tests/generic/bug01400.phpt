--TEST--
Test for PHP-1400: Default writeConcern not write safe on standalone node
--SKIPIF--
<?php $needs = "2.8.0-RC4"; $needsOp = "ge"; ?>
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function log_cmd_insert($server, $document, $options, $prot_options) {
    echo "Issuing cmd_insert\n";
	var_dump($options['writeConcern']);
}

$ctx = stream_context_create(array(
    'mongodb' => array(
        'log_cmd_insert' => 'log_cmd_insert',
    ),
));

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));
$col = $m->selectCollection("test", "collection");
 
$col->remove();
 
$col->insert(array(
     "name" => "Next promo",
     "inprogress" => false,
     "priority" => 0,
     "tasks" => array( "select product", "add inventory", "do placement"),
) );
?>
--EXPECT--
Issuing cmd_insert
array(1) {
  ["w"]=>
  int(1)
}
