--TEST--
Test for PHP-522: Setting per-insert options. (streams)
--SKIPIF--
<?php if (!MONGO_STREAMS) { echo "skip This test requires streams support"; } ?>
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/replicaset.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";
require_once "tests/utils/stream-notifications.inc";

function dump_writeOptions(MongoNotifications $mn) {
    $meta = $mn->getLastInsertMeta();
    var_dump($meta['write_options']);
}

$mn = new MongoNotifications;
$ctx = stream_context_create(
    array(),
    array(
        "notification" => array($mn, "update")
    )
);

$rs = MongoShellServer::getReplicasetInfo();
$m = new MongoClient($rs['dsn'], array('replicaSet' => $rs['rsname']), array("context" => $ctx));
$c = $m->selectCollection( dbname(), "php-522_error_26" );

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'socketTimeoutMS' => "1" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
dump_writeOptions($mn);
echo "-----\n";

try {
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "1", 'safe' => 1, 'w' => 4, 'socketTimeoutMS' => "1foo" ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
dump_writeOptions($mn);
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => 1, 'socketTimeoutMS' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
dump_writeOptions($mn);
echo "-----\n";

try {
	$c->w = 2;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => "yesplease", 'safe' => 5, 'socketTimeoutMS' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
dump_writeOptions($mn);
echo "-----\n";

try {
	$c->w = 2;
	$c->wtimeout = 4500;
	$retval = $c->insert( array( 'test' => 1 ), array( 'fsync' => false, 'safe' => "allDCs", 'socketTimeoutMS' => M_PI * 1000 ) );
	var_dump($retval["ok"]);
} catch ( Exception $e ) {
	echo $e->getMessage(), "\n";
}
dump_writeOptions($mn);
echo "-----\n";
?>
--EXPECTF--
%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %s on line %d
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
array(1) {
  ["writeConcern"]=>
  array(3) {
    ["fsync"]=>
    bool(true)
    ["j"]=>
    bool(false)
    ["w"]=>
    int(4)
  }
}
-----

%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %s on line %d
%s:%d: Read timed out after reading 0 bytes, waited for 0.001000 seconds
array(1) {
  ["writeConcern"]=>
  array(3) {
    ["fsync"]=>
    bool(true)
    ["j"]=>
    bool(false)
    ["w"]=>
    int(4)
  }
}
-----

%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %s on line %d

%s: MongoCollection::insert(): Using w=2 rather than w=1 as suggested by deprecated 'safe' value in %s on line %d
float(1)
array(1) {
  ["writeConcern"]=>
  array(3) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["w"]=>
    int(2)
  }
}
-----

%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %s on line %d
%s:%d: Read timed out after reading 0 bytes, waited for 3.141000 seconds
array(1) {
  ["writeConcern"]=>
  array(3) {
    ["fsync"]=>
    bool(true)
    ["j"]=>
    bool(false)
    ["w"]=>
    int(5)
  }
}
-----

%s: MongoCollection::insert(): The 'safe' option is deprecated, please use 'w' instead in %s on line %d
%s:%d:%sunrecognized getLastError mode: allDCs
array(1) {
  ["writeConcern"]=>
  array(4) {
    ["fsync"]=>
    bool(false)
    ["j"]=>
    bool(false)
    ["wtimeout"]=>
    int(4500)
    ["w"]=>
    string(6) "allDCs"
  }
}
-----

