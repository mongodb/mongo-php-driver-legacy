--TEST--
Test for bug PHP-407 MongoBinData should default to 0, not 2
--SKIPIF--
<?php require dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
new MongoBinData("data");
?>
--EXPECTF--
%s: MongoBinData::__construct(): The default value for type will change to 0 in the future. Please pass in '2' explicitly. in %sbug00407.php on line %d
