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
class MongoUtil
{
  
    /**
     * Less than.
     */
    const LT = '$lt';

    /**
     * Less than or equal to.
     */
    const LTE = '$lte';

    /**
     * Greater than.
     */
    const GT = '$gt';

    /**
     * Greater than or equal to.
     */
    const GTE = '$gte';

    /**
     * Checks for a field in an object.
     */
    const IN = '$in';

    /**
     * Not equal.
     */
    const NE = '$ne';


    /**
     * Sort ascending.
     */
    const ASC = 1;

    /**
     * Sort descending.
     */
    const DESC = -1;


    /**
     * Function as binary data.
     */
    const BIN_FUNCTION = 1;

    /**
     * Default binary type: an arrray of binary data.
     */
    const BIN_ARRAY = 2;

    /**
     * Universal unique id.
     */
    const BIN_UUID = 3;

    /**
     * Binary MD5.
     */
    const BIN_MD5 = 5;

    /**
     * User-defined binary type.
     */
    const BIN_CUSTOM = 128;


    /* Command collection */
    protected static $CMD = '.$cmd';

    /* Admin database */
    const ADMIN = "admin";

    /* Commands */
    const AUTHENTICATE      = "authenticate";
    const CREATE_COLLECTION = "create";
    const DELETE_INDICES    = "deleteIndexes";
    const DROP              = "drop";
    const DROP_DATABASE     = "dropDatabase";
    const FORCE_ERROR       = "forceerror";
    const INDEX_INFO        = "cursorInfo";
    const LAST_ERROR        = "getlasterror";
    const LIST_DATABASES    = "listDatabases";
    const LOGGING           = "opLogging";
    const LOGOUT            = "logout";
    const NONCE             = "getnonce";
    const PREV_ERROR        = "getpreverror";
    const PROFILE           = "profile";
    const QUERY_TRACING     = "queryTraceLevel";
    const REPAIR_DATABASE   = "repairDatabase";
    const RESET_ERROR       = "reseterror";
    const SHUTDOWN          = "shutdown";
    const TRACING           = "traceAll";
    const VALIDATE          = "validate";

}
