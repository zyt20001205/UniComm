---@meta

---@class Ftp
Ftp = {}

---
---Create an FTP client instance and connect to the server.
---
---[FTP demo](../demo/ftp.lua)
---
---SSL client ports establish implicit TLS before the server greeting. Temporary data sockets follow the port type.
---Call `login` to authenticate and, for SSL ports, enable TLS protection for data connections.
---
---Once a transfer is accepted, its final control reply is checked even when data I/O fails.
---If completion cannot be confirmed, the control transport is closed and an error
---is raised. Reconnect before retrying; the stored port reference is retained.
---
---@param name portName Target TCP or SSL client port name.
---@param timeout? integer (default: 30000) Timeout in **milliseconds** for blocking FTP operations.
---@return ftp
function Ftp.new(name, timeout) end

---@class ftp
ftp = {}

---
---Authenticate with the FTP server, sending PASS only when requested.
---
---For SSL client ports, successful authentication is followed by `PBSZ 0` and `PROT P`.
---Both commands must receive a `200` reply before this method returns normally.
---
---@param username string FTP username.
---@param password password FTP password.
---@return nil
function ftp:login(username, password) end

---
---Return the current remote working directory.
---
---Quoted paths are unescaped, including doubled quotes inside the path.
---For an unquoted `257` reply, the complete reply text is treated as the path.
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
---Try MLSD first and return structured `FileInfo` entries when supported.
---If the server rejects MLSD with `500` or `502`, retry with LIST and return
---raw lines as strings instead. Only line endings are removed; filenames,
---spacing, headers and timestamps are not parsed. Empty lines are omitted.
---
---For a non-empty result, `type(entries[1])` is `"table"` for MLSD or `"string"`
---for LIST. An empty result is an empty table in either case.
---Transport errors and other server errors raise a Lua error without fallback.
---
---@param path? string (default: "") Remote directory path; when omitted uses the current directory.
---@return FileInfo[]|string[] entries Structured MLSD entries, or raw LIST lines after fallback.
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
---Use MLST first: `250` returns true and `550` returns false.
---If MLST is rejected with `500` or `502`, try SIZE: `213` returns true and
---`550` returns false. The SIZE fallback does not probe directories.
---A `550` reply is treated as unavailable; it may also reflect access restrictions.
---Other SIZE replies raise an error.
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
