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
    protected $file;

    /**
     * Create a new GridFS file.  
     * These should usually be created by MongoGridFS.
     *
     * @param gridfile $file a file from the database
     */
    public function __construct($file) 
    {
        $this->file = $file;
    }

    /**
     * Returns this file's filename.
     *
     * @return string the filename
     */
    public function getFilename() 
    {
        return mongo_gridfile_filename($this->file);
    }

    /**
     * Returns this file's size.
     *
     * @return int the file size
     */
    public function getSize() 
    {
        return mongo_gridfile_size($this->file);
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
        if (!$filename) {
            $filename = $this->getFilename();
        }
        return mongo_gridfile_write($this->file, $filename);
    }
}

?>
