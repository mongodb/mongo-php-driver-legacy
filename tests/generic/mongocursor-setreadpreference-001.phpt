--TEST--
MongoCursor::setReadPreference() should set read preference mode
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$modes = array(
    Mongo::RP_PRIMARY,
    Mongo::RP_PRIMARY_PREFERRED,
    Mongo::RP_SECONDARY,
    Mongo::RP_SECONDARY_PREFERRED,
    Mongo::RP_NEAREST
);

foreach (array_values($modes) as $mode) {
    $m = new_mongo(null, true, true, array('readPreference' => $mode));
    $c = $m->phpunit->test->find();
    echo $mode, "\n\n";
    foreach (array_values($modes) as $newMode) {
        $c->setReadPreference($newMode);
        $rp = $c->getReadPreference();
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
