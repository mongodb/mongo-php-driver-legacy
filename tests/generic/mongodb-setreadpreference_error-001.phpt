--TEST--
MongoDB::setReadPreference() error setting invalid read preference mode
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
--FILE--
<?php require_once "tests/utils/server.inc"; ?>
<?php

$modes = array("blaat", 42, true, 3.14);

foreach ($modes as $mode) {
    $m = new_mongo_standalone(null, true, true, array('readPreference' => MongoClient::RP_PRIMARY_PREFERRED));
    $db = $m->phpunit;
    $db->setReadPreference($mode);
    $rp = $db->getReadPreference();
    echo $rp["type"], "\n";
}
?>
--EXPECTF--
Warning: MongoDB::setReadPreference(): The value 'blaat' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoDB::setReadPreference(): The value '42' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoDB::setReadPreference(): The value '1' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoDB::setReadPreference(): The value '3.14' is not valid as read preference type in %s on line %d
primaryPreferred
