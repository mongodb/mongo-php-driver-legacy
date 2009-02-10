<?php

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

  /**
   * Converts a field or array of fields into an underscore-separated string.
   * @param string|array $keys field(s) to convert
   * @return string the index name
   */
  public static function to_index_string( $keys ) {
    if( is_string( $keys ) ) {
      $name = str_replace( ".", "_", $keys ) + "_1";
    }
    else {
      $key_list = array();
      foreach( $keys as $v ) {
        $key_list[] = str_replace( ".", "_", $v ) + "_1";
      }
      $name = implode( "_", $key_list );
    }
    return $name;
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
  public static $ADMIN = "admin";

  /* Commands */
  public static $AUTHENTICATE = "authenticate";
  public static $CREATE_COLLECTION = "create";
  public static $DELETE_INDICES = "deleteIndexes";
  public static $DROP = "drop";
  public static $DROP_DATABASE = "dropDatabase";
  public static $LAST_ERROR = "getlasterror";
  public static $LIST_DATABASES = "listDatabases";
  public static $LOGGING = "opLogging";
  public static $LOGOUT = "logout";
  public static $NONCE = "getnonce";
  public static $PREV_ERROR = "getpreverror";
  public static $PROFILE = "profile";
  public static $QUERY_TRACING = "queryTraceLevel";
  public static $REPAIR_DATABASE = "repairDatabase";
  public static $RESET_ERROR = "reseterror";
  public static $SHUTDOWN = "shutdown";
  public static $TRACING = "traceAll";
  public static $VALIDATE = "validate";

  public static $LT = "\$lt";
  public static $LTE = "\$lte";
  public static $GT = "\$gt";
  public static $GTE = "\$gte";
  public static $IN = "\$in";
  public static $NE = "\$ne";

}

?>
