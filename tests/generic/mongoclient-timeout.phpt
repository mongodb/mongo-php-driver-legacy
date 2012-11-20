--TEST--
Test for database timeout option.
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
$m = mongo("admin");
$d = $m->selectDb('admin');

try
{
	$d->command(array('sleep' => true, 'secs' => 2), array('timeout' => 1));
}
catch ( MongoCursorTimeoutException $e )
{
	var_dump( $e );
}
sleep( 2 );
?>
--EXPECTF--
object(MongoCursorTimeoutException)#%d (%d) {
  ["message%S:protected%S]=>
  string(%s) "%s:%d: cursor timed out (timeout: 1, time left: 0:%d, status: 0)"
  ["string%S:private%S]=>
  string(0) ""
  ["code%S:protected%S]=>
  int(80)
  ["file%S:protected%S]=>
  string(%d) "%s"
  ["line%S:protected%S]=>
  int(%d)
  ["trace%S:private%S]=>
  array(1) {
    [0]=>
    array(6) {
      ["file"]=>
      string(%d) "%s"
      ["line"]=>
      int(%d)
      ["function"]=>
      string(7) "command"
      ["class"]=>
      string(7) "MongoDB"
      ["type"]=>
      string(2) "->"
      ["args"]=>
      array(2) {
        [0]=>
        array(2) {
          ["sleep"]=>
          bool(true)
          ["secs"]=>
          int(2)
        }
        [1]=>
        array(1) {
          ["timeout"]=>
          int(1)
        }
      }
    }
  }
  ["previous%S:private%S]=>
  NULL
  ["host":"MongoCursorException":private]=>
  NULL
  ["fd":"MongoCursorException":private]=>
  int(0)
}
