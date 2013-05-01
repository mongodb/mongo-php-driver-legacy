--TEST--
Test for PHP-795: MongoCode segfaults when internal 'code' property is modified
--FILE--
<?php
function errh($errno, $errmsg) {
    echo $errmsg, "\n";
    return true;
}
set_error_handler("errh", E_RECOVERABLE_ERROR);


$m = new MongoCode("bacd");
$m->code = new stdclass;
var_dump((string)$m);
?>
--EXPECTF--
Object of class stdClass could not be converted to string

Notice: Object of class stdClass to string conversion in %sbug00795.php on line %d
string(6) "Object"
