--TEST--
MongoCommandCursor::setReadPreference() should set read preference mode
--SKIPIF--
<?php require_once __DIR__ . "/skipif.inc"; ?>
--FILE--
<?php

$modes = array(
    MongoClient::RP_PRIMARY,
    MongoClient::RP_PRIMARY_PREFERRED,
    MongoClient::RP_SECONDARY,
    MongoClient::RP_SECONDARY_PREFERRED,
    MongoClient::RP_NEAREST
);

foreach (array_values($modes) as $mode) {
    $mc = new MongoClient(null, array('connect' => false, 'readPreference' => $mode));
    $cc = new MongoCommandCursor($mc, 'test.foo', array());
    echo $mode, "\n\n";
    foreach (array_values($modes) as $newMode) {
        $cc->setReadPreference($newMode);
        $rp = $cc->getReadPreference();
        echo $rp["type"], "\n";
    }
    echo "---\n";
}

?>
===DONE===
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
===DONE===
