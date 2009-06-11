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
 * Handy methods for programming with this driver.
 * 
 * @category Database
 * @package  Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */
class MongoGridFSFileClassic extends MongoGridFSFile
{

    /**
     * Creates a classic-version gridfs file.
     *
     * @param MongoGridFSFile $f a GridFS 1.0 file
     */
    public function __construct(MongoGridFSFile $f) {
        $this->gridfs = $f->gridfs;
        $this->file = $f->file;
    }

    /**
     * Returns this file's contents as a string of bytes.
     *
     * @return string returns a string of the bytes in the file.
     */
    public function getBytes() {
        $str = "";
        if (!array_key_exists('next', $this->file)) {
            return $str;
        }

        $n = $this->file['next'];
        $chunk = $this->gridfs->getDBRef($n);

        while ($chunk != NULL) {
            $str .= $chunk['data']->bin;

            if (array_key_exists('next', $chunk)) {
                $chunk = $this->gridfs->getDBRef($chunk['next']);
            }
            else {
                break;
            }
        }

        return $str;
    }

    /**
     * Writes this file to the filesystem.
     *
     * @param string $filename the name of the file to write
     * 
     * @return int the number of bytes written
     */
    public function write($filename = NULL) {
        $written = 0;

        if ($filename == NULL) {
            $filename = $this->file['filename'];
        }

        $fd = fopen($filename, 'w');
        if (!$fd) {
            throw new MongoGridFSException("Couldn't open file $filename for writing");
        }

        if (!array_key_exists('next', $this->file)) {
            fclose($fd);
            return $written;
        }

        $chunk = $this->gridfs->getDBRef($this->file['next']);

        while ($chunk != NULL) {
            fwrite($fd, $chunk['data']->bin);
            $written += strlen($chunk['data']->bin);

            if (array_key_exists('next', $chunk)) {
                $chunk = $this->gridfs->getDBRef($chunk['next']);
            }
            else {
                break;
            }
        }

        fclose($fd);
        return $written;
    }
}


