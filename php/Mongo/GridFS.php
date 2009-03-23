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

require_once "Mongo/GridFS/File.php";
require_once "Mongo/Cursor.php";

/**
 * Utilities for storing and retreiving files from the database.
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoGridFS
{
    protected $resource;
    protected $prefix;
    protected $db;

    /**
     * Creates a new gridfs instance.
     *
     * @param MongoDB $db     database
     * @param string  $prefix optional files collection prefix
     */
    public function __construct($db, $prefix = "fs") 
    {
        $this->resource = mongo_gridfs_init($db->connection, $db->name, $prefix);
        $this->prefix   = $prefix;
        $this->db       = $db;
    }

    /**
     * Lists all files matching a given criteria.
     *
     * @param array $query criteria to match
     *
     * @return mongo_cursor cursor over the list of files
     */
    public function listFiles($query = null) 
    {
        if (is_null($query)) {
            $query = array();
        }
        return MongoCursor::getGridFSCursor(mongo_gridfs_list($this->resource, 
                                                              $query));
    }

    /**
     * Stores a file in the database.
     *
     * @param string $filename the name of the file
     *
     * @return mongo_id the database id for the file
     */
    public function storeFile($filename) 
    {
        return mongo_gridfs_store($this->resource, $filename);
    }

    /**
     * Retreives a file from the database.
     *
     * @param array|string $query the filename or criteria for which to search
     *
     * @return mongo_gridfs_file the file
     */
    public function findFile($query) 
    {
        if (is_string($query)) {
            $query = array("filename" => $query);
        }
        return new MongoGridFSFile(mongo_gridfs_find($this->resource, $query));
    }

    /**
     * Saves an uploaded file to the database.
     *
     * @param string $name     the name field of the uploaded file
     * @param string $filename the name to give the file when saved in the database
     *
     * @return mongo_id the id of the uploaded file
     */
    public function storeUpload($name, $filename=null) 
    {
        if (!$name || !is_string($name) ||
            !$_FILES || !$_FILES[ $name ]) {
            return false;
        }

        $tmp = $_FILES[ $name ]["tmp_name"];
        if ($filename) {
            $name = "$filename";
        } else {
            $name = $_FILES[ $name ]["name"];
        }

        $this->storeFile($tmp);

        // make the filename more paletable
        $coll              = $this->db->selectCollection($this->prefix . ".files");
        $obj               = $coll->findOne(array("filename" => $tmp));
        $obj[ "filename" ] = $name;
        $coll->update(array("filename" => $tmp), $obj);

        return $obj[ "_id" ];
    }
}

?>
