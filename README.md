# Cloud-backup

It is divided into Windows client and Linux server.

The client provides automatic check and upload function of file modification.

The server provides non hot compression of files, browser browsing and downloading and Breakpoint renewal.

Technology realization：

server: The server uses boost library and HTTP library, uses read-write lock to realize safe multithreading, uses zip library to compress 
files in non hot store, and unordered map data structure to provide efficient data management.

client: The client uses the boost library to check the Etag value of the file, so as to realize the automatic backup of the file.
