--TEST--
MongoCollection::insert() with ReplicaSet failover.
--DESCRIPTION--
Here we test whether the ping is only done once every 5 seconds.
--SKIPIF--
skip Manual test
--FILE--
<?php
function error_handler($code, $message)
{
	echo $message, "\n";
}

set_error_handler('error_handler');

MongoLog::setLevel(MongoLog::ALL);
MongoLog::setModule(MongoLog::ALL);

//require_once dirname(__FILE__) . "/../utils.inc";

$mongo = new Mongo("mongodb://whisky:13002,whisky:13001/?replicaSet=seta");
$mongo->safe = true;
$mongo->setReadPreference(Mongo::RP_NEAREST);

$coll1 = $mongo->selectCollection('phpunit', 'query');
$coll1->drop();

$i = 0;
while ($i < 5) {
	echo "Inserting $i\n";
	try {
		$coll1->insert(array('_id' => $i, 'x' => "foo" . dechex($i)), array('safe' => 1));
		$i++;
	} catch ( Exception $e ) {
		echo get_class( $e ), ': ', $e->getCode(), ', ', $e->getMessage(), "\n";
	}
	sleep(1);
}
?>
--EXPECT--
