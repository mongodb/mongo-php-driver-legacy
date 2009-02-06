<?php

dl("libmongo.so");

include "mongo_auth.php";

class mongo {

  var $host = "localhost";
  var $port = "27017";
  var $connection = NULL;

  /** Creates a new database connection.
   * @param string $host the host name (optional)
   * @param int $port the db port (optional)
   */
  public function __construct( $host = "localhost", $port = NULL ) {
    $addr = $host;
    $this->host = $host;
    if( $port ) {
      $addr .= ":$port";
      $this->port = $port;
    }

    $this->connection = mongo_connect( $addr );
  }

  public function __toString() {
    return $this->host . ":" . $this->port;
  }

  /** 
   * Gets a database.
   * @param string $dbname the database name
   * @return mongo_db a new db object
   */
  public function select_database( $dbname = NULL ) {
    if( $dbname == NULL || $dbname == "" ) {
      trigger_error( "Invalid database name.", E_USER_WARNING );
      return false;
    }
    return new mongo_db( $this, $dbname );
  }

  /** 
   * Lists all of the databases.
   * @return Array each database with its size and name
   */
  public function list_databases() {
    $data = array( mongo_util::$LIST_DATABASES => 1 );
    $result = mongo_util::db_command( $this->connection, $data );
    if( $result )
      return $result[ "databases" ];
    else
      return false;
  }

  /**
   * Drops a database.
   * @param mongo_db $db the database to drop
   * @return array db response
   */
  public function drop_database( mongo_db $db ) {
    return $db->drop();
  }

  /**
   * Repairs and compacts a database.
   * @param mongo_db $db the database to drop
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails
   * @param bool $backup_original_files if original files should be backed up
   * @return array db response
   */
  public function repair_database( mongo_db $db, $preserve_cloned_files = false, $backup_original_files = false ) {
    return $db->repair( $preserve_cloned_files, $backup_original_files );
  }

  /**
   * Closes this database connection.
   * @return bool if the connection was successfully closed
   */
  public function close() {
    mongo_close( $this->connection );
  }

}

class mongo_db {

  var $connection = NULL;
  var $name = NULL;

  public function __construct( mongo $conn, $name ) {
    $this->connection = $conn->connection;
    $this->name = $name;
  }

  public function __toString() {
    return $this->name;
  }


  public function get_auth( $username, $password ) {
    return mongo_auth::get_auth( $this->connection, $this->name, $username, $password );
  }

  /** 
   * Returns the name of the database currently in use.
   * @return string the name of the database
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Gets this database's profiling level.
   * @return int the profiling level
   */
  public function get_profiling_level() {
    $data = array( mongo_util::$PROFILE => -1 );
    $x = mongo_util::db_command( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 )
      return $x[ "was" ];
    else
      return false;
  }

  /**
   * Sets this database's profiling level.
   * @return int the old profiling level
   */
  public function set_profiling_level( $level ) {
    $data = array( mongo_util::$PROFILE => (int)$level );
    $x = mongo_util::db_command( $this->connection, $data, $this->name );
    if( $x[ "ok" ] == 1 ) {
      return $x[ "was" ];
    }
    return false;
  }

  /**
   * Drops this database.
   * @return array db response
   */
  public function drop() {
    $data = array( mongo_util::$DROP_DATABASE => $this->name );
    return mongo_util::db_command( $this->connection, $data );
  }

  /**
   * Repairs and compacts this database.
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails (optional)
   * @param bool $backup_original_files if original files should be backed up (optional)
   * @return array db response
   */
  function repair( $preserve_cloned_files = false, $backup_original_files = false ) {
    $data = array( mongo_util::$REPAIR_DATABASE => 1 );
    if( $preserve_cloned_files )
      $data[ "preserveClonedFilesOnFailure" ] = true;
    if( $backup_original_files )
      $data[ "backupOriginalFiles" ] = true;
    return mongo_util::db_command( $this->connection, $data, $this->name );
  }


  /** 
   * Gets a collection.
   * @param string $name the name of the collection
   */
  public function select_collection( $name ) {
    return new mongo_collection( $this, $name );
  }

  /** 
   * Creates a collection.
   * @param string $name the name of the collection
   * @param bool $capped if the collection should be a fixed size
   * @param int $size if the collection is fixed size, its size in bytes
   * @param int $max if the collection is fixed size, the maximum 
   *     number of elements to store in the collection
   * @return Collection a collection object representing the new collection
   */
  public function create_collection( $name, $capped = false, $size = 0, $max = 0 ) {
    $data = array( mongo_util::$CREATE_COLLECTION => $name );
    if( $capped && $size ) {
      $data[ "capped" ] = true;
      $data[ "size" ] = $size;
      if( $max )
        $data[ "max" ] = $max;
    }

    mongo_util::db_command( $this->connection, $data );
    return new mongo_collection( $this, $name );
  }

  /**
   * Drops a collection.
   * @param mongo_collection $coll
   * @return array the db response
   */
  public function drop_collection( mongo_collection $coll ) {
    return $coll->drop();
  }

}


class mongo_collection {

  var $name = "";
  var $db;
  var $connection;

  function __construct( mongo_db $db, $name ) {
    $this->connection = $db->connection;
    $this->db = $db->name;
    $this->name = $name;
  }

  public function __toString() {
    return $this->db . "." . $this->name;
  }

  /**
   * Drops this collection.
   * @return array the db response
   */
  function drop() {
    $data = array( mongo_util::$DROP => $this->name );
    return mongo_util::db_command( $this->connection, $data, $this->db );
  }

  /**
   * Validates this collection.
   * @param bool $scan_data only validate indices, not the base collection (optional)
   * @return array the database's evaluation of this object
   */
  function validate( $scan_data = false ) {
    $data = array( mongo_util::$VALIDATE => $this->name );
    if( $scan_data )
      $data[ "scandata" ] = true;
    return mongo_util::db_command( $this->connection, $data, $this->db );
  }

  /** Inserts an object or array into the collection.
   * @param object $iterable an object or array
   * @return array the associative array saved to the database
   */
  function insert( $iterable ) {
    $arr = mongo_util::obj_to_array( $iterable );
    $result = mongo_insert( $this->connection, (string)$this, $arr );
    if( $result )
      return $iterable;
    return false;
  }

  /** 
   * Querys this collection.
   * @param array $query the fields for which to search
   * @param int $skip number of results to skip
   * @param int $limit number of results to return
   * @param array $fields fields of each result to return
   * @return mongo_cursor a cursor for the search results
   */
  function find( $query = NULL, $skip = NULL, $limit = NULL, $fields = NULL ) {
    return new mongo_cursor( $this->connection, (string)$this, $query, $skip, $limit, $fields );
  }

}


class mongo_cursor {

  var $connection = NULL;

  private $cursor = NULL;
  private $started_iterating = false;

  private $query = NULL;
  private $fields = NULL;
  private $limit = NULL;
  private $skip = NULL;

  public function __construct( $conn, $ns, $query = NULL, $skip = NULL, $limit = NULL, $fields = NULL ) {
    $this->connection = $conn;
    $this->ns = $ns;
    $this->query = $query;
    $this->skip = $skip;
    $this->limit = $limit;
    $this->fields = $fields;
  }

  /**
   * Return the next object to which this cursor points, and advance the cursor.
   * @return array the next object
   */
  public function next() {
    if( !$this->started_iterating ) {
      $this->do_query();
      $this->started_iterating = true;
    }

    return mongo_next( $this->cursor );
  }

  /**
   * Checks if there are any more elements in this cursor.
   * @return bool if there is another element
   */
  public function has_next() {
    if( !$this->started_iterating ) {
      $this->do_query();
      $this->started_iterating = true;
    }

    return mongo_has_next( $this->cursor );
  }

  /**
   * Limits the number of results returned.
   * @param int $num the number of results to return
   * @return mongo_cursor this cursor
   */
  public function limit( $num ) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->limit = (int)$num;
    return $this;
  }

  /**
   * Skips a number of results.
   * @param int $num the number of results to skip
   * @return mongo_cursor this cursor
   */
  public function skip( $num) {
    if( $this->started_iterating ) {
      trigger_error( "cannot modify cursor after beginning iteration.", E_USER_ERROR );
      return false;
    }
    $this->skip = (int)$num;
    return $this;
  }

  /**
   * Execute the query and set the cursor resource.
   */
  private function do_query() {
    $q = mongo_util::obj_to_array( $this->fields );
    if( !is_null( $this->fields ) ) {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$skip, (int)$limit, mongo_util::obj_to_array( $fields ) );
    }
    else if( !is_null( $limit ) ) {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$skip, (int)$limit );
    }
    // 0 means the same as NULL for skip
    else if( $skip ) {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q, (int)$skip );
    }
    else {
      $this->cursor = mongo_query( $this->connection, $this->ns, $q );
    }
  }
}


class mongo_util {
  
  /**
   * Turns something into an array that can be saved to the db.
   * Returns the empty array if passed NULL.
   * @param any $obj object to convert
   * @return array the array
   */
  public static function obj_to_array( $obj ) {
    if( is_null( $obj ) ) {
      return array();
    }
    if( is_array( $obj ) ) {
      return $obj;
    }
    $arr = array();
    foreach( $obj as $key=>$value ) {
      $arr[ $key ] = $value;
    }
    return $arr;
  }


  /** Execute a db command
   * @param array $data the query to send
   * @param string $db the database name
   */
  public static function db_command( $conn, $data, $db = NULL ) {
    // check if dbname is set
    if( !$db ) {
      // default to admin?
      $db = mongo_util::$ADMIN;
    }

    $cmd_collection = $db . mongo_util::$CMD;
    $obj = mongo_find_one( $conn, $cmd_collection, $data );

    if( $obj ) {
      return $obj;
    }
    else {
      trigger_error( "no response?", E_USER_WARNING );
      return false;
    }
  }

  /* Command collection */
  private static $CMD = ".\$cmd";
  /* Admin database */
  private static $ADMIN = "admin";

  /* Commands */
  public static $AUTHENTICATE = "authenticate";
  public static $CREATE_COLLECTION = "create";
  public static $DROP = "drop";
  public static $DROP_DATABASE = "dropDatabase";
  public static $LIST_DATABASES = "listDatabases";
  public static $NONCE = "getnonce";
  public static $PROFILE = "profile";
  public static $REPAIR_DATABASE = "repairDatabase";
  public static $VALIDATE = "validate";

}

?>
