--TEST--
GridFS: Testing fseek and fread (2)
--SKIPIF--
<?php require_once dirname(__FILE__) ."/skipif.inc"; ?>
--FILE--
<?php
require_once dirname(__FILE__) . "/../utils.inc";
function readRange($fp, $seek, $length = false)
{
	fseek($fp, $seek, SEEK_SET);
	$data = '';
	if ($length === false) {
		while (!feof($fp)) {
			$buffer = fread($fp, 8192);
			$data .= $buffer;
		}
	} else {
		$toRead = $length - $seek;
		while ($toRead > 0) {
			$buffer = fread($fp, $toRead);
			$toRead -= strlen($buffer);
			$data .= $buffer;
		}
	}
	return $data;
}

$m = Mongo();
$db = $m->selectDb('phpunit');
$grid = $db->getGridFS('wrapper');
$grid->drop();
$grid->storeFile('tests/brighton-save.osm');

$file = $grid->findOne(array('filename' => 'tests/brighton-save.osm'));
echo $file->file['chunkSize'], "\n";
$fp = $file->getResource();

echo md5(readRange($fp, 0, 819300)), "\n";
echo md5(readRange($fp, 0, 819300)), "\n";
$first = readRange($fp, 0, 819300);
echo md5(readRange($fp, 819300, false)), "\n";
echo md5(readRange($fp, 819300, false)), "\n";
echo md5(readRange($fp, 819300, false)), "\n";
echo md5(readRange($fp, 819300, false)), "\n";
$second = readRange($fp, 819300, false);
echo md5_file('tests/brighton-save.osm'), "\n";
echo md5($first . $second), "\n";
?>
--EXPECT--
262144
3c1fbf79189651e9e0d81058cd3c6af6
3c1fbf79189651e9e0d81058cd3c6af6
856bebb64ad591da27e61a9d288a0dce
856bebb64ad591da27e61a9d288a0dce
856bebb64ad591da27e61a9d288a0dce
856bebb64ad591da27e61a9d288a0dce
1769daba434b221ac2577e4f6f491cca
1769daba434b221ac2577e4f6f491cca
