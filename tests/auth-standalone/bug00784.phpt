--TEST--
Test for PHP-784: Administrative commands unnecessarily require database authentication
--SKIPIF--
<?php require_once "tests/utils/auth-standalone.inc" ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$db = dbname();

$s = new MongoShellServer();
$host = $s->getStandaloneConfig(true);
$s->addStandaloneUser($db, 'bug00784', 'foobar');
$creds = $s->getCredentials();

$options = array(
    'db' => $db,
    'username' => 'bug00784',
    'password' => 'foobar',
);

$m = new MongoClient($host, $options);

$c = $m->selectCollection($db, 'bug00784');
$c->drop();
$c->insert(array('x' => 1));

var_dump($c->count());

try {
    $m->admin->command(array(
        'renameCollection' => "$db.bug00784",
        'to' => "$db.bug00784_renamed",
        'dropTarget' => true,
    ));
} catch (MongoConnectionException $e) {
    printf("error message: %s\n", $e->getMessage());
    printf("error code: %d\n", $e->getCode());
}

var_dump($c->count());

$c = $m->selectCollection($db, 'bug00784_renamed');
var_dump($c->count());
?>
--EXPECTF--
int(1)
int(0)
int(1)
