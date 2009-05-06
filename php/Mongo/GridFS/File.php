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

/**
 * Utilities for getting information about files from the database.
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoGridFSFile
{
    public $file;
    protected $gridfs;

    /**
     * Create a new GridFS file.  
     * These should usually be created by MongoGridFS.
     *
     * @param gridfile $file a file from the database
     */
    public function __construct(MongoGridFS $gridfs, $file) 
    {
        $this->gridfs = $gridfs;
        $this->file = $file;
    }

    /**
     * Returns this file's filename.
     *
     * @return string the filename
     */
    public function getFilename() 
    {
        return $this->file['filename'];
    }

    /**
     * Returns the number of bytes in this file..
     *
     * @return int the number of bytes in this file
     */
    public function getSize() 
    {
        return $this->file['length'];
    }

    /**
     * Writes this file to the filesystem.
     *
     * @param string $filename the location to which to write the file
     *
     * @return int the number of bytes written
     */
    public function write($filename = null) 
    {
        // make sure that there's an index on chunks so we can sort by chunk num
        $this->gridfs->chunks->ensureIndex(array("n" => 1));

        if (!$filename) {
            $filename = $this->getFilename();
        }
        $query = array("query" => array('files_id' => $this->file['_id']), 
                       "orderby" => array("n" => 1));
        return mongo_gridfile_write($this->gridfs->resource, $query, $filename);
    }

    /**
     * Gets the raw byte of a file.
     * Warning: this will load the file into memory.  If the file is 
     * bigger than your memory, this will cause problems!  
     *
     * @return bytes a string of the bytes in the file
     */
    public function getBytes()
    {
        $this->gridfs->chunks->ensureIndex(array("n" => 1));
        $cursor = $this->gridfs->chunks->find(array('files_id' => $this->file['_id']))->sort(array('n' => 1));

        $str = '';
        foreach ($cursor as $chunk) {
            $str .= $chunk['data'];
        }
        return $str;
    }
}

?>
