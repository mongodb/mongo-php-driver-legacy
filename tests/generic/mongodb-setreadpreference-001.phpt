--TEST--
MongoDb::setReadPreference [1]
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
	$m = $c->phpunit;
	echo $value, "\n\n";
	foreach ($b as $newRP) {
		$m->setReadPreference($newRP);
		$rp = $m->getReadPreference();
		echo $rp["type_string"], "\n";
	}
	echo "---\n";
}
?>
--EXPECT--
primary

primary
primary preferred
secondary
secondary preferred
nearest
---
primaryPreferred

primary
primary preferred
secondary
secondary preferred
nearest
---
secondary

primary
primary preferred
secondary
secondary preferred
nearest
---
secondaryPreferred

primary
primary preferred
secondary
secondary preferred
nearest
---
nearest

primary
primary preferred
secondary
secondary preferred
nearest
---
