<?php
function findCollection($d, $name)
{
	$cols = $d->getCollectionInfo(true);
	if (!array_key_exists($name, $cols)) {
		return NULL;
	} else {
		$cols[$name]['name'] = dbname() . '.' . $cols[$name]['name'];
		return $cols[$name];
	}
}
?>
