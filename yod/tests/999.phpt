--TEST--
Check for yod clean
--SKIPIF--
<?php if (!extension_loaded("yod")) print "skip"; ?>
--FILE--
<?php 
echo "yod clean";
?>
--CLEAN--
<?php include 'clean.php'; ?>
--EXPECT--
yod clean
