--TEST--
Connection strings: read preferences [1]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db	= dbname();

$baseString = sprintf("mongodb://%s:%d/%s?readPreference=", $host, $port, $db);

$a = array(
	'primary', 'PrIMary',
	'primaryPreferred',	'primarypreferred', 'PRIMARYPREFERRED',
	'secondary', 'SECONdary',
	'secondaryPreferred', 'SecondaryPreferred',
	'NEAREST', 'nearesT',
	'nonsense'
);

foreach ($a as $value) {
	$m = new mongo($baseString . $value);
	$rp = $m->getReadPreference();
	echo $rp["type"], "\n";
}
?>
--EXPECTF--
primary
primary
primaryPreferred
primaryPreferred
primaryPreferred
secondary
secondary
secondaryPreferred
secondaryPreferred
nearest
nearest

Fatal error: Uncaught exception 'MongoConnectionException' with message 'The readPreference value 'nonsense' is not supported.' in %smongoclient-ctor-readPref-001.php:%d
Stack trace:
#0 %smongoclient-ctor-readPref-001.php(%d): Mongo->__construct('%s')
#1 {main}
  thrown in %smongoclient-ctor-readPref-001.php on line %d
