--TEST--
Mongo::setReadPreference [1]
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
	$m = new mongo($baseString . $value);
	echo $value, "\n\n";
	foreach ($b as $newRP) {
		$m->setReadPreference($newRP);
		$rp = $m->getReadPreference();
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
