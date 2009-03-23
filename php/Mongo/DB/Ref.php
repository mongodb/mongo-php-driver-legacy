<?php
/**
 *  Copyright 2009 10gen, Inc.
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * PHP version 5 
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */

require_once "Mongo/DB.php";

/**
 * This class can be used to create lightweight links between objects
 * in different collections.
 *
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoDBRef
{
    protected static $_refKey = '$ref';
    protected static $_idKey  = '$id';
     
    /**
     * Creates a new db reference.
     *
     * @param string $ns the name of the collection
     * @param mixed  $id the _id of the object
     *
     * @return array a new db ref
     */ 
    public static function create($ns, $id) 
    {
        return array(MongoDBRef::$_refKey => "$ns",
                     MongoDBRef::$_idKey => $id);
    }

    /**
     * Checks if an object is a db ref
     *
     * @param array $obj object to check
     * 
     * @return bool if the object is a db ref
     */
    public static function isRef($obj) 
    {
        if (is_array($obj) && 
            array_key_exists(MongoDBRef::$_refKey, $obj) &&
            array_key_exists(MongoDBRef::$_idKey, $obj)) {
            return true;
        }
        return false;
    }

    /**
     * Gets the value a db ref points to.
     *
     * @param MongoDB $db  database to use
     * @param array   $ref database reference
     *
     * @return array the object the db ref points to or null
     */
    public static function get(MongoDB $db, $ref) 
    {
        return $db->selectCollection($ref[MongoDBRef::$_refKey])->
          findOne(array("_id" => $ref[MongoDBRef::$_idKey]));
    }
}

?>
