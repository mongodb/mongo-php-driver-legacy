<?php

dl("libmongo.so");

$x = temp( array( "str" => "foo", 
             "lnum" => 20, 
             "dnum" => 3.14,
             "nval" => NULL,
             1 => 6,
             "bval" => FALSE,
             "aval" => array( "foo" => "bar", "abc" => "xyz" ),
             0 => "hello"
             ) );

echo "my output:\n";
foreach( $x as $key => $value) {
  echo "$key : $value\n";
}

?>
