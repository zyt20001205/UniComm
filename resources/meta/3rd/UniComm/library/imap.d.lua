---@meta

---@class Imap
Imap = {}

---
---Create an IMAP instance, open the configured port and read the server greeting.
---
---[IMAP demo](../demo/imap.lua)
---
---@param name portName
---@param timeout? integer (default: 1000) Maximum time in **milliseconds** to wait for data to arrive.
---@return imap
function Imap.new(name, timeout) end

---@class (exact) ImapBody
---@field contentType string MIME content type.
---@field data string Body content after supported transfer decoding.

---@class (exact) ImapAttachment
---@field name string Attachment file name.
---@field contentType string MIME content type.
---@field data string Binary content after supported transfer decoding.

---@class (exact) ImapMail
---@field header table<string, string> Message header fields.
---@field body ImapBody[] Message body parts.
---@field attachments ImapAttachment[] Message attachments.

---@class imap
imap = {}

---
---Send LOGIN command to authenticate with IMAP server.
---
---@param username string IMAP username/email address.
---@param password password IMAP application password.
---@return nil
function imap:login(username, password) end

---
---Wait for a new message in INBOX, optionally filter by sender, then return its parsed content.
---
---@param from? string (default: "") Expected sender; when omitted accept any sender.
---@param timeout? integer (default: 600000) Maximum total time in **milliseconds** to wait for mail to arrive.
---@return ImapMail mail Parsed message content.
function imap:receive(from, timeout) end

---
---Send LOGOUT command and close the IMAP session.
---
---@return nil
function imap:logout() end
