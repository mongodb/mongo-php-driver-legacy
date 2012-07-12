--TEST--
Bug#00407
--SKIPIF--
<?php require __DIR__ ."/skipif.inc"; ?>
--FILE--
<?php
new MongoBinData("data");
?>
--EXPECTF--
Deprecated: MongoBinData::__construct(): The default value for type will change to 0 in the future. Please pass in '0' explicitly. in %sbug00407.php on line %d
