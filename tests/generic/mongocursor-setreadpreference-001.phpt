--TEST--
MongoCursor::setReadPreference [1]
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
<?php
$host = hostname();
$port = port();
$db   = dbname();

$baseString = sprintf("mongodb://%s:%d/%s?readPreference=", $host, $port, $db);

$a = array(
	'primary',
	'primaryPreferred',
	'secondary',
	'secondaryPreferred',
	'nearest'
);
$b = array(
	Mongo::RP_PRIMARY,
	Mongo::RP_PRIMARY_PREFERRED,
	Mongo::RP_SECONDARY,
	Mongo::RP_SECONDARY_PREFERRED,
	Mongo::RP_NEAREST
);

foreach ($a as $value) {
	$c = new mongo($baseString . $value);
	$d = $c->phpunit;
	$m = $d->test;
	$cur = $m->find();
	echo $value, "\n\n";
	foreach ($b as $newRP) {
		$cur->setReadPreference($newRP);
		$rp = $cur->getReadPreference();
		echo $rp["type"], "\n";
	}
	echo "---\n";
}
?>
--EXPECT--
primary

primary
primaryPreferred
secondary
secondaryPreferred
nearest
---
primaryPreferred

primary
primaryPreferred
secondary
secondaryPreferred
nearest
---
secondary

primary
primaryPreferred
secondary
secondaryPreferred
nearest
---
secondaryPreferred

primary
primaryPreferred
secondary
secondaryPreferred
nearest
---
nearest

primary
primaryPreferred
secondary
secondaryPreferred
nearest
---
