# MyFileSystem

This is a simple file system. Here arrays are used to create an abstraction of hard disk. The file system is implemented on this abstract hard disk. The various operations that can be performed are as follows:

   1.  myfs> /* prompt given by this program */
   2.  myfs> **mkfs** drive_name block_size total_size /* creates a filesystem named **drive_name**, with specified **block_size in Bytes** and **total_size in MB**  */
   3.  myfs> **use** drive_name as other_name /* the filesystem on **drive_name** will henceforth be accessed as **other_name** */
   4.  myfs> **cp** source_file drive_name\dest_file /* copy the file **source_file** from OS to the filesystem **drive_name** as **dest_file** */
   5.  myfs> **cp** drive_1\source_file drive_2\dest_file /* copy the file **source_file** from **drive_1** to the filesystem **drive_2** as **dest_file** */
   6.  myfs> **ls** drive_name /* see the contents of the filesystem **drive_name** */
  7.  myfs> **rm** drive_name\file_name /* Delete the **file_name** from **drive_name** */
  8.  myfs> **mv** drive_1\source_file drive_2\dest_file  /* move the file **source_file** from **drive_1** to the filesystem **drive_2** as **dest_file** */
  9.  myfs> **create** drive_name file_name /*Create a **file_name** in the **drive_name** */
  10.  myfs> **write** "drive_name\file_name" /*Write to **file_name** in **drive_name** */
  11.  myfs> **display** "drive_name\file_name" /*Display content of **file_name** in **drive_name** */
  12.  myfs> **exit** /* terminate the process */
