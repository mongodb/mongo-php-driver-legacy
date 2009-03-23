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

require_once "Mongo.php";
include "PEAR/Exception.php";

/**
 * General Mongo exception class.
 *
 * @category Database
 * @package  DB_Mongo
 * @author   Kristina Chodorow <kristina@10gen.com>
 * @license  http://www.apache.org/licenses/LICENSE-2.0  Apache License 2
 * @link     http://www.mongodb.org
 */

if (class_exists(PEAR_Exception)) {
    class MongoException extends PEAR_Exception
    {
        /**
         * Create a new exception.
         *
         * @param string $message exception message
         * @param int    $code    exception code
         */
        public function __construct($message, $code=Mongo::ERR_GENERAL) 
        {
            parent::__construct($message, $code);
        }

        /**
         * Returns the string form of this exception.
         *
         * @return string this exception
         */
        public function __toString() 
        {
            return __CLASS__ . ": [{$this->code}]: {$this->message}\n";
        }
    }
}
else {
    class MongoException extends Exception
    {
        /**
         * Create a new exception.
         *
         * @param string $message exception message
         * @param int    $code    exception code
         */
        public function __construct($message, $code=Mongo::ERR_GENERAL) 
        {
            parent::__construct($message, $code);
        }

        /**
         * Returns the string form of this exception.
         *
         * @return string this exception
         */
        public function __toString() 
        {
            return __CLASS__ . ": [{$this->code}]: {$this->message}\n";
        }
    }
}

?>
