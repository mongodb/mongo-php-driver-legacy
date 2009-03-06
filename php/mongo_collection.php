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
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @version  CVS: 000000
 * @link     http://www.mongodb.org
 */

/**
 * Represents a collection of documents in the database.
 * 
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoCollection
{

    var $parent;
    var $connection;
    var $db;
    var $name = "";

    /**
     * Creates a new collection.
     *
     * @param MongoDB $db   database
     * @param string  $name collection name
     */
    function __construct(MongoDB $db, $name)
    {
        $this->parent     = $db;
        $this->connection = $db->connection;
        $this->db         = $db->name;
        $this->name       = $name;
    }

    /**
     * String representation of this collection.
     *
     * @return string the full name of this collection
     */
    public function __toString() 
    {
        return $this->db . "." . $this->name;
    }

    /**
     * Drops this collection.
     *
     * @return array the db response
     */
    function drop() 
    {
        $this->deleteIndexes();
        $data = array(MongoUtil::$DROP => $this->name);
        return MongoUtil::dbCommand($this->connection, $data, $this->db);
    }

    /**
     * Validates this collection.
     *
     * @param bool $scan_data only validate indices, not the base collection
     *
     * @return array the database's evaluation of this object
     */
    function validate($scan_data = false) 
    {
        $data = array(MongoUtil::$VALIDATE => $this->name);
        if ($scan_data) {
            $data[ "scandata" ] = true;
        }
        return MongoUtil::dbCommand($this->connection, $data, $this->db);
    }

    /** Inserts an object or array into the collection.
     *
     * @param object $iterable an object or array
     *
     * @return array the associative array saved to the database
     */
    function insert($iterable) 
    {
        $arr    = $iterable;
        $result = mongo_insert($this->connection, (string)$this, $arr);
        if ($result) {
            return $iterable;
        }
        return false;
    }

    /** Inserts many objects into the database at once.
     *
     * @param array $a an array of objects or arrays
     *
     * @return array the array saved to the database
     */
    function batchInsert($a = array()) 
    {
        if (!count($a)) {
            return $a;
        }
        $result = mongo_batch_insert($this->connection, (string)$this, $a);
        if ($result) {
            return $a;
        }
        return false;
    }

    /** 
     * Querys this collection.
     *
     * @param object $query  the fields for which to search
     * @param object $fields fields of each result to return
     *
     * @return mongo_cursor a cursor for the search results
     */
    function find($query = array(), $fields = array()) 
    {
        return new MongoCursor($this->connection, 
                               (string)$this, 
                               $query, 
                               $fields);
    }

    /** 
     * Querys this collection, returning a single element.
     *
     * @param object $query the fields for which to search
     *
     * @return object a record matching the search or null
     */
    function findOne($query = array()) 
    {
        return mongo_find_one($this->connection, 
                              (string)$this, 
                              $query);
    }

    /**
     * Update records based on a given criteria.
     * <!--Options:
     * <dl>
     * <dt>upsert : bool</dt>
     * <dd>if $newobj should be inserted if no matching records are found</dd>
     * <dt>ids : bool </dt>
     * <dd>if if the _id field should be added in the case of an upsert</dd>
     * </dl>-->
     *
     * @param object $criteria description of the objects to update
     * @param object $newobj   the object with which to update the matching records
     * @param bool   $upsert   if the object should be inserted if the criteria isn't found
     *
     * @return array the associative array saved to the db
     */
    function update($criteria, $newobj, $upsert = false) 
    {
        $c      = $criteria;
        $obj    = $newobj;
        $result = mongo_update($this->connection, (string)$this, $c, $obj, $upsert);
        if ($result) {
            return $obj;
        }
        return false;
    }

    /**
     * Remove records from this collection.
     *
     * @param object $criteria description of records to remove
     * @param bool   $just_one remove at most one record matching this criteria
     *
     * @return bool if the command was executed successfully
     */
    function remove($criteria = array(), $just_one = false) 
    {
        return mongo_remove($this->connection, 
                            (string)$this, 
                            $criteria, 
                            $just_one);
    }

    /**
     * Creates an index on the given field(s), or does nothing if the index 
     * already exists.
     *
     * @param string|array $keys field or fields to use as index
     *
     * @return void
     */
    function ensureIndex($keys) 
    {
        $ns = (string)$this;
        if (is_string($keys)) {
            $keys = array($keys => 1);
        }
        $name = MongoUtil::toIndexString($keys);
        $coll = $this->parent->selectCollection("system.indexes");
        $coll->insert(array("ns" => $ns, "key" => $keys, "name" => $name));
    }
  
    /**
     * Deletes an index from this collection.
     *
     * @param string|array $keys field or fields from which to delete the index
     *
     * @return void
     */
    function deleteIndex($keys) 
    {
        $idx = MongoUtil::toIndexString($key);
        $d   = array(MongoUtil::$DELETE_INDICES => $this->name, "index" => $idx);
        $x   = MongoUtil::dbCommand($this->connection, $d, $this->db);    
    }

    /**
     * Delete all indices for this collection.
     * 
     * @return void
     */
    function deleteIndexes() 
    {
        $d = array(MongoUtil::$DELETE_INDICES => $this->name, "index" => "*");
        $x = MongoUtil::dbCommand($this->connection, $d, $this->db);
    }

    /**
     * Returns an array of index names for this collection.
     *
     * @return array index names
     */
    function getIndexInfo() 
    {
        $ns           = (string)$this;
        $nscollection = $this->parent->selectCollection("system.indexes");
        $cursor       = $nscollection->find(array("ns" => $ns));
        $a            = array();
        while ($cursor->hasNext()) {
            $obj = $cursor->next();
            unset($obj[ "id" ]);
            unset($obj[ "ns" ]);
            array_push($a, $cursor->next());
        }
        return $a;
    }

    /**
     * Counts the number of documents in this collection.
     *
     * @return int the number of documents
     */
    function count() 
    {
        $result = MongoUtil::dbCommand($this->connection, 
                                       array("count" => $this->name), 
                                       $this->db);
        if ($result) {
            return $result[ "n" ];
        }
        trigger_error("count failed", E_USER_ERROR);
    }

    /**
     * Saves an object to this collection.
     *
     * @param object $obj object to save
     *
     * @return object the object saved
     */
    function save($obj) 
    {
        $a = $obj;
        if ($a[ "_id" ]) {
            return $this->update(array("_id" => $id), $a, true);
        }
        return $this->update($a, $a, true);
    }
}

?>
