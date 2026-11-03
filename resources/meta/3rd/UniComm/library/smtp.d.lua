---@meta

---@class Smtp
Smtp = {}

---
---Create an SMTP instance, open the configured port and read the server greeting.
---
---[SMTP demo](../demo/smtp.lua)
---
---@param name portName
---@param timeout? integer (default: 1000) Maximum time in **milliseconds** to wait for data to arrive.
---@return smtp
function Smtp.new(name, timeout) end

---@class (exact) SmtpMail
---@field from string Sender email address.
---@field to string|string[] Primary recipient or list of recipients.
---@field cc? string|string[] Carbon-copy recipient or list of recipients.
---@field bcc? string|string[] Blind-carbon-copy recipient or list of recipients.
---@field subject string Message subject.
---@field body string Message body.
---@field attachment? string|string[] Attachment path or list of attachment paths.

---@class smtp
smtp = {}

---
---Send AUTH LOGIN command to authenticate with SMTP server.
---
---@param username string SMTP username/email address.
---@param password password SMTP password.
---@return nil
function smtp:authLogin(username, password) end

---
---Send EHLO (Extended Hello) command to SMTP server to initiate session and discover server capabilities.
---
---@return nil
function smtp:ehlo() end

---
---Send an email with MAIL, RCPT and DATA commands.
---
---@param mail SmtpMail
---@return nil
function smtp:send(mail) end

---
---Send QUIT command to SMTP server to end communication.
---
---@return nil
function smtp:quit() end
