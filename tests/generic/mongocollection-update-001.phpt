--TEST--
MongoCollection::update() routes unacknowledged write through legacy op code
--SKIPIF--
<?php $needs = "2.5.5"; ?>
<?php require_once "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

function stream_notify($notificationCode, $severity, $message, $messageCode, $bytesTransferred, $bytesMax) {
    if ($notificationCode != MONGO_STREAM_NOTIFY_TYPE_LOG) {
        return;
    }

    $data = unserialize($message);

    switch ($messageCode) {
        case MONGO_STREAM_NOTIFY_LOG_CMD_UPDATE:
            printf("Issuing update command. Write concern: %s\n", json_encode($data['write_options']['writeConcern']));
            break;

        case MONGO_STREAM_NOTIFY_LOG_UPDATE:
            printf("Issuing OP_UPDATE. Write concern: %s\n", json_encode($data['options']));
            break;
    }
}

$ctx = stream_context_create(
    array(),
    array('notification' => 'stream_notify')
);

$host = MongoShellServer::getStandaloneInfo();
$m = new MongoClient($host, array(), array('context' => $ctx));

$c = $m->selectCollection(dbname(), collname(__FILE__));
$c->drop();

echo "Acknowledged writes will use the update command:\n";
$c->update(array('x' => 1), array('$set' => array('y' => 1)));
$c->update(array('x' => 1), array('$set' => array('y' => 1)), array('w' => 1));
$c->update(array('x' => 1), array('$set' => array('y' => 1)), array('w' => 0, 'j' => true));

echo "\nUnacknowledged writes will use OP_UPDATE:\n";
$c->update(array('x' => 1), array('$set' => array('y' => 1)), array('w' => 0));
$c->w = 0;
$c->update(array('x' => 1), array('$set' => array('y' => 1)));

?>
==DONE==
--EXPECTF--
Acknowledged writes will use the update command:
Issuing update command. Write concern: {"w":1}
Issuing update command. Write concern: {"w":1}
Issuing update command. Write concern: {"j":true,"w":0}

Unacknowledged writes will use OP_UPDATE:
Issuing OP_UPDATE. Write concern: {"w":0}
Issuing OP_UPDATE. Write concern: []
==DONE==
