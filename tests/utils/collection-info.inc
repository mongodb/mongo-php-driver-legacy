<?php

function findCollection($db, $name)
{
    $collections = $db->getCollectionInfo(array('includeSystemCollections' => true));

    return isset($collections[$name]) ? $collections[$name] : null;
}
