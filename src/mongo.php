<?php

dl("libmongo.so");

class mongo extends mongo_api {

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
    parent::__construct( $this->connection );
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
    $data = array( mongo_api::$LIST_DATABASES => 1 );
    $result = $this->db_command( $data );
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

class mongo_db extends mongo_api{

  var $name = NULL;

  public function __construct( mongo $conn, $name ) {
    parent::__construct( $conn->connection );
    $this->$name = $name;
  }

  public function __toString() {
    return $this->name;
  }

  /** 
   * Returns the name of the database currently in use.
   * @return string the name of the database
   */
  public function get_name() {
    return $this->name;
  }

  /**
   * Drops this database.
   * @return array db response
   */
  public function drop() {
    $data = array( mongo_api::$DROP_DATABASE => $this->name );
    return $this->db_command( $data );
  }

  /**
   * Repairs and compacts this database.
   * @param bool $preserve_cloned_files if cloned files should be kept if the repair fails (optional)
   * @param bool $backup_original_files if original files should be backed up (optional)
   * @return array db response
   */
  function repair( $preserve_cloned_files = false, $backup_original_files = false ) {
    $data = array( mongo_api::$REPAIR_DATABASE => 1 );
    if( $preserve_cloned_files )
      $data[ "preserveClonedFilesOnFailure" ] = true;
    if( $backup_original_files )
      $data[ "backupOriginalFiles" ] = true;
    return $this->db_command( $data, $this->name );
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
    $data = array( mongo_api::$CREATE_COLLECTION => $name );
    if( $capped && $size ) {
      $data[ "capped" ] = true;
      $data[ "size" ] = $size;
      if( $max )
        $data[ "max" ] = $max;
    }

    $this->db_command( $data );
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


class mongo_collection extends mongo_api {

  var $name = "";
  private $db;

  function __construct( mongo_db $db, $name ) {
    parent::__construct( $db->connection );
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
    $data = array( mongo_api::$DROP => $this->name );
    return $this->db_command( $data, $this->db );
  }

  /**
   * Validates this collection.
   * @param bool $scan_data only validate indices, not the base collection (optional)
   * @return array the database's evaluation of this object
   */
  function validate( $scan_data = false ) {
    $data = array( mongo_api::$VALIDATE => $this->name );
    if( $scan_data )
      $data[ "scandata" ] = true;
    return $this->db_command( $data, $this->db );
  }

}


class mongo_cursor {
  var $cursor = NULL;

}


class mongo_api {

  var $connection = NULL;

  /** Creates a new access point to the api
   * @param resource $conn a mongo db connection
   */
  public function __construct( $conn ) {
    $this->connection = $conn;
  }
  
  /** Execute a db command
   * @param array $data the query to send
   * @param string $db the database name
   */
  public function db_command( $data, $db = NULL ) {
    // check if dbname is set
    if( !$db ) {
      // default to admin?
      $db = mongo_api::$ADMIN;
    }

    $cmd_collection = $db . mongo_api::$CMD;
    $obj = mongo_find_one( $this->connection, $cmd_collection, $data );

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
  public static $CREATE_COLLECTION = "create";
  public static $DROP = "drop";
  public static $DROP_DATABASE = "dropDatabase";
  public static $LIST_DATABASES = "listDatabases";
  public static $REPAIR_DATABASE = "repairDatabase";
  public static $VALIDATE = "validate";

}

?>
