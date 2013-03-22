--TEST--
Test for PHP-554: MongoId should not get constructed when passing in an invalid ID.
--SKIPIF--
<?php require "tests/utils/standalone.inc";?>
--FILE--
<?php
require_once "tests/utils/server.inc";

$valid = array(
    str_repeat("abcdef123456", 2),
    new MongoId,
);

$invalid = array(
    str_repeat("klsdjf", 4),
    str_repeat("abcdef123456", 2). " ",
    new stdclass,
);

echo "VALID IDs\n";
foreach($valid as $id) {
    var_dump($id);
    var_dump(new MongoId($id));
}

echo "INVALID IDs:\n";
foreach($invalid as $id) {
    try {
        var_dump($id);
        var_dump(new MongoId($id));
    } catch(MongoException $e) {
        var_dump($e->getMessage(), $e->getCode());
    }
}

?>
--EXPECTF--
VALID IDs
string(24) "abcdef123456abcdef123456"
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "abcdef123456abcdef123456"
}
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "%s"
}
object(MongoId)#%d (1) {
  ["$id"]=>
  string(24) "%s"
}
INVALID IDs:
string(24) "klsdjfklsdjfklsdjfklsdjf"
string(31) "ID must be valid hex characters"
int(18)
string(25) "abcdef123456abcdef123456 "
string(17) "Invalid object ID"
int(19)
object(stdClass)#%d (0) {
}
string(17) "Invalid object ID"
int(19)

