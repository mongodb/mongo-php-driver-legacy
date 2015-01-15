<?php

function findCollection($db, $name)
{
    $collections = $db->getCollectionInfo(array(
        'includeSystemCollections' => true,
        'filter' => array('name' => $name),
    ));

    return reset($collections) !== false ? current($collections) : null;
}
