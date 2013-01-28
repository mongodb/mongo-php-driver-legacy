--TEST--
MongoCursor::setReadPreference() error setting invalid read preference mode
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php require_once dirname(__FILE__) . "/../utils.inc"; ?>
<?php

$modes = array("blaat", 42, true, 3.14);

foreach ($modes as $mode) {
    $m = new_mongo(null, true, true, array('readPreference' => MongoClient::RP_PRIMARY_PREFERRED));
    $c = $m->phpunit->test->find();
    $c->setReadPreference($mode);
    $rp = $c->getReadPreference();
    echo $rp["type"], "\n";
}
?>
--EXPECTF--
Warning: MongoCursor::setReadPreference(): The value 'blaat' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoCursor::setReadPreference(): The value '42' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoCursor::setReadPreference(): The value '1' is not valid as read preference type in %s on line %d
primaryPreferred

Warning: MongoCursor::setReadPreference(): The value '3.14' is not valid as read preference type in %s on line %d
primaryPreferred
