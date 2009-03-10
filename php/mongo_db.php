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
 * Instances of this class are used to interact with a database.
 * To get a database:
 * <pre>
 *   $m = new Mongo(); // connect
 *   $db = $m->selectDatabase(); // get a database object
 * </pre>
 * 
 * @category DB
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoDB
{

    var $connection = null;
    var $name       = null;

    /**
     * Creates a new database.
     * 
     * @param Mongo  $conn connection
     * @param string $name database name
     */
    public function __construct(Mongo $conn, $name) 
    {
        $this->connection = $conn->connection;
        $this->name       = $name;
    }

    /**
     * The name of this database.
     *
     * @return string this database's name
     */
    public function __toString() 
    {
        return $this->name;
    }

    /**
     * Fetches toolkit for dealing with files stored in this database.
     *
     * @param string $prefix name of the file collection
     *
     * @return gridfs a new gridfs object for this database
     */
    public function getGridfs($prefix = "fs") 
    {
        return new MongoGridfs($this, $prefix);
    }

    /**
     * Gets this database's profiling level.
     *
     * @return int the profiling level
     */
    public function getProfilingLevel() 
    {
        $data = array(MongoUtil::$PROFILE => -1);
        $x    = MongoUtil::dbCommand($this->connection, $data, $this->name);
        if ($x[ "ok" ] == 1) {
            return $x[ "was" ];
        } else {
            return false;
        }
    }

    /**
     * Sets this database's profiling level.
     *
     * @param int $level profiling level
     *
     * @return int the old profiling level
     */
    public function setProfilingLevel($level) 
    {
        $data = array(MongoUtil::$PROFILE => (int)$level);
        $x    = MongoUtil::dbCommand($this->connection, $data, $this->name);
        if ($x[ "ok" ] == 1) {
            return $x[ "was" ];
        }
        return false;
    }

    /**
     * Drops this database.
     *
     * @return array db response
     */
    public function drop() 
    {
        $data = array(MongoUtil::$DROP_DATABASE => $this->name);
        return MongoUtil::dbCommand($this->connection, $data);
    }

    /**
     * Repairs and compacts this database.
     *
     * @param bool $preserve_cloned_files if cloned files should be kept if the 
     *     repair fails
     * @param bool $backup_original_files if original files should be backed up
     *
     * @return array db response
     */
    function repair($preserve_cloned_files = false, $backup_original_files = false) 
    {
        $data = array(MongoUtil::$REPAIR_DATABASE => 1);
        if ($preserve_cloned_files) {
            $data[ "preserveClonedFilesOnFailure" ] = true;
        }
        if ($backup_original_files) {
            $data[ "backupOriginalFiles" ] = true;
        }
        return MongoUtil::dbCommand($this->connection, $data, $this->name);
    }


    /** 
     * Gets a collection.
     *
     * @param string $name the name of the collection
     *
     * @return MongoCollection the collection
     */
    public function selectCollection($name) 
    {
        return new MongoCollection($this, $name);
    }

    /** 
     * Creates a collection.
     *
     * @param string $name   the name of the collection
     * @param bool   $capped if the collection should be a fixed size
     * @param int    $size   if the collection is fixed size, its size in bytes
     * @param int    $max    if the collection is fixed size, the maximum 
     *     number of elements to store in the collection
     *
     * @return Collection a collection object representing the new collection
     */
    public function createCollection($name, $capped = false, $size = 0, $max = 0) 
    {
        $data = array(MongoUtil::$CREATE_COLLECTION => $name);
        if ($capped && $size) {
            $data[ "capped" ] = true;
            $data[ "size" ]   = $size;
            if ($max) {
                $data[ "max" ] = $max;
            }
        }

        MongoUtil::dbCommand($this->connection, $data, $this->name);
        return new MongoCollection($this, $name);
    }

    /**
     * Drops a collection.
     *
     * @param mongo_collection $coll collection to drop
     *
     * @return array the db response
     */
    public function dropCollection(MongoCollection $coll) 
    {
        return $coll->drop();
    }

    /**
     * Get a list of collections in this database.
     *
     * @return array a list of collection names
     */
    public function listCollections() 
    {
        $nss     = $this->selectCollection("system.namespaces")->find();
        $ns_list = array();
        while ($nss->hasNext()) {
            $ns = $nss->next();
            $ns = $ns[ "name" ];
            if (strpos($ns, "\$") > 0) {
                continue;
            }
            array_push($ns_list, $ns);
        }
        return $ns_list;
    }

    /**
     * Gets information from the database about cursors.
     * Returns an array of the form:
     * <pre>
     * array(3) {
     *   ["byLocation_size"]=>
     *   int(...)
     *   ["clientCursors_size"]=>
     *   int(...)
     *   ["ok"]=>
     *   float(1)
     * }
     * </pre>
     *
     * @return array information about the cursor
     */
    public function getCursorInfo()
    {
        $a = array(MongoUtil::$INDEX_INFO => 1);
        return MongoUtil::dbCommand($this->connection, $a, "$this");
    }

}

define("MONGO_PROFILING_OFF", 0);
define("MONGO_PROFILING_SLOW", 1);
define("MONGO_PROFILING_ON", 2);


?>
