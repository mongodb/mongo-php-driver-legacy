--TEST--
MongoRegex constructor given invalid arguments
--SKIPIF--
<?php require_once dirname(__FILE__) . "/skipif.inc"; ?>
--FILE--
<?php
$regexes = array('', '/', '345', 'b');

foreach ($regexes as $regex) {
    try {
        new MongoRegex($regex);
    } catch (Exception $e) {
        printf("%s: %d\n", get_class($e), $e->getCode());
    }
}
?>
--EXPECT--
MongoException: 9
MongoException: 9
MongoException: 9
MongoException: 9
