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
require_once "Mongo/GridFS/Cursor.php";

/**
 * Utilities for storing and retreiving files from the database.
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoGridFS extends MongoCollection
{
    public $resource;

    // chunks collection
    public $chunks;

    // names
    protected $files_name;
    protected $chunks_name;

    /**
     * Creates new file collections.
     *
     * Files are stored across two collections, the first containing file meta 
     * information, the second containing chunks of the actual files.  By default,
     * fs.files and fs.chunks are the collections used.  
     *
     * Use one option to specify a prefix other that fs:
     * <pre>
     * $fs = new MongoGridFS($db, "myfiles");
     * </pre>
     * Uses myfiles.files and myfiles.chunks collections.
     *
     * Use two options to fully specify the two collection names:
     * <pre>
     * $fs = new MongoGridFS($db, "myfiles", "mychunks");
     * </pre>
     * Uses myfiles and mychunks collections.
     *
     * @param MongoDB $db     database
     * @param string  $files  optional files collection name, if chunks is not given,
     *                        files is used as a collection prefix
     * @param string  $chunks optional chunks collection name
     */
    public function __construct(MongoDB $db, $files = null, $chunks = null) 
    {
        if (is_null($files) && is_null($chunks)) {
            $this->files_name = "fs.files";
            $this->chunks_name = "fs.chunks";
        }
        else if (is_null($chunks)) {
            $this->files_name = "$files.files";
            $this->chunks_name = "$files.chunks";
        }
        else {
            $this->files_name = $files;
            $this->chunks_name = $chunks;
        }

        parent::__construct($db, $this->files_name);

        $this->chunks   = new MongoCollection($db, $this->chunks_name);
        // make sure that there's an index on chunks so we can sort by chunk num
        $this->chunks->ensureIndex(array("n" => 1));

        $this->resource = mongo_gridfs_init($db->connection, $db->name, $this->files_name, $this->chunks_name);
        $this->db       = $db;
    }


    /**
     * Drops the files and chunks collections.
     *
     * @return array the db response
     */
    function drop() 
    {
        MongoUtil::dbCommand($this->db->connection, 
                             array(MongoUtil::DROP => $this->chunks->name), 
                             (string)$this->db);
        return MongoUtil::dbCommand($this->db->connection, 
                                    array(MongoUtil::DROP => $this->name), 
                                    (string)$this->db);
    }

    /**
     * Lists all files matching a given criteria.
     * Each result is returned as a GridFSFile object.
     *
     * @param array $query criteria to match
     *
     * @return GridFSCursor cursor over the list of files
     *
     * @throws InvalidArgumentException if the parameters 
     *         passed are not arrays
     */
    public function find($query = array()) 
    {
        if (!is_array($query)) {
            throw new InvalidArgumentException("Expects: find(array)");
        }

        return new MongoGridFSCursor($this,
                                     $this->db->connection,
                                     (string)$this,
                                     $query,
                                     array());
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
     * @return GridFSFile the file
     */
    public function findOne($query = array()) 
    {
        if (is_string($query)) {
            $query = array("filename" => $query);
        }
        return $this->find($query)->limit(1)->getNext();
    }

    /**
     * Remove matching files from the collection.
     * This also removes all associated chunks.
     *
     * @param object $criteria description of files to remove
     * @param bool   $just_one remove at most one file matching this criteria
     *
     * @return bool if the command was executed successfully
     */
    function remove($criteria = array(), $just_one = false) 
    {
        $cursor = parent::find($criteria, array("_id" => 1));
        foreach ($cursor as $v) {
            $this->chunks->remove(array("files_id" => $v['_id']));
        }
        return mongo_remove($this->db->connection, 
                            (string)$this, 
                            $criteria, 
                            (bool)$just_one);
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
        $obj               = parent::findOne(array("filename" => $tmp));
        $obj[ "filename" ] = $name;
        $this->update(array("filename" => $tmp), $obj);

        return $obj[ "_id" ];
    }
}

?>
