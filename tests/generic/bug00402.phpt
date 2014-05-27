--TEST--
Test for PHP-402: MongoCollection::validate(true) doesn't set the correct scan-all flag.
--DESCRIPTION--
This test skips mongos because its validate() results contain only a top-level
"validate" field, which collects the shard results, and grouped results for each
shard in a "raw" array field.
--SKIPIF--
<?php require_once "tests/utils/standalone.inc"; ?>
<?php require_once dirname(__FILE__) . '/skipif_mongos.inc'; ?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$m = mongo_standalone();
$c = $m->selectCollection('phpunit', 'col');
$c->insert(array('x' => 1), array('w' => true));

$result = $c->validate();
var_dump(isset($result['warning']));
var_dump($result['warning']);

$result = $c->validate(true);
var_dump(isset($result['warning']));

$c->drop();
$res = $c->validate();
var_dump($res["ok"], $res["errmsg"]);
?>
--EXPECTF--
bool(true)
string(79) "Some checks omitted for speed. use {full:true} option to do more thorough scan."
bool(false)
float(0)
string(%d) "%s not found"

