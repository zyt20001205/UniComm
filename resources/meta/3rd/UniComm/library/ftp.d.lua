---@meta

---@class Ftp
Ftp = {}

---
---Create an FTP client instance and connect to the server.
---
---[FTP demo](../demo/ftp.lua)
---
---SSL client ports establish implicit TLS before the server greeting. Temporary data sockets follow the port type.
---Data-channel protection negotiation (`PBSZ` / `PROT`) is not performed automatically.
---
---@param name portName Target TCP or SSL client port name.
---@param timeout? integer (default: 30000) Timeout in **milliseconds** for blocking FTP operations.
---@return ftp
function Ftp.new(name, timeout) end

---@class ftp
ftp = {}

---
---Send USER and PASS commands to authenticate with FTP server.
---
---@param username string FTP username.
---@param password password FTP password.
---@return nil
function ftp:login(username, password) end

---
---Return the current remote working directory.
---
---@return string path Current remote directory path.
function ftp:pwd() end

---
---Change the current remote working directory.
---
---@param path string Remote directory path.
---@return nil
function ftp:cd(path) end

---
---List remote directory entries through a temporary passive data connection.
---
---@param path? string (default: "") Remote directory path; when omitted uses the current directory.
---@return FileInfo[]
function ftp:list(path) end

---
---Return facts about one remote file or directory through the control connection.
---
---@param path string Remote file or directory path.
---@return FileInfo
function ftp:stat(path) end

---
---Check whether a remote file or directory is available.
---
---@param path string Remote file or directory path.
---@return boolean
function ftp:exists(path) end

---
---Create a remote directory.
---
---@param path string Remote directory path.
---@return nil
function ftp:mkdir(path) end

---
---Remove an empty remote directory.
---
---@param path string Remote directory path.
---@return nil
function ftp:rmdir(path) end

---
---Delete a remote file.
---
---@param path string Remote file path.
---@return nil
function ftp:delete(path) end

---
---Rename a remote file or directory.
---
---@param from string Current remote path.
---@param to string New remote path.
---@return nil
function ftp:rename(from, to) end

---
---Download a remote file in binary mode.
---
---@param path string Remote file path.
---@return string data Binary file content.
function ftp:download(path) end

---
---Create or overwrite a remote file with binary data.
---
---@param path string Remote file path.
---@param data string Binary file content.
---@return nil
function ftp:upload(path, data) end

---
---Send QUIT command and close the FTP session.
---
---@return nil
function ftp:quit() end
