-- Configure a TCP client port named "FTP" and use an account with write permission.
-- SSL client ports are also accepted for implicit TLS; temporary data sockets
-- follow the control port type.
local ftp = Ftp.new("FTP", 30000)

-- SSL login also sends PBSZ 0 and PROT P to enable TLS on data connections.
ftp:login("anonymous", "anonymous@example.com")

-- PWD and other metadata commands use the persistent FTP control connection.
-- pwd() accepts quoted paths and unquoted replies such as "257 /".
print("remote working directory", ftp:pwd())

-- Work inside a disposable remote directory so existing files are not modified.
local directory = "unicomm-ftp-demo-" .. os.time()
ftp:mkdir(directory)
ftp:cd(directory)

-- Transfer commands open a temporary passive data connection automatically.
-- EPSV is preferred and PASV is used as the fallback.
-- If completion cannot be confirmed, the control connection is closed; reconnect before retrying.
-- A transfer's final control reply is checked before returning, even on data errors.
ftp:upload("hello.txt", "Hello from UniComm FTP!\n")

-- exists() tries SIZE if MLST is rejected with 500/502; SIZE 213 confirms a file.
-- SIZE 550 returns false; other errors still raise. No directory fallback is provided.
-- print("hello.txt exists", ftp:exists("hello.txt"))
-- print("missing file exists", ftp:exists("missing-" .. directory .. ".txt"))

-- stat() still requires MLST support; skip this example when MLST is unsupported.
-- local entry = ftp:stat("hello.txt")
-- print(entry.name, entry.type, entry.size, entry.modified)

-- list() prefers MLSD and returns FileInfo entries when it succeeds.
-- If MLSD is rejected with 500/502, LIST is retried and returns raw string lines.
-- A raw line is not a filename: permissions, sizes, dates and spaces stay intact.
local entries = ftp:list()
for _, item in ipairs(entries) do
    if type(item) == "string" then
        print(item)
    else
        print(item.name, item.type, item.size, item.modified)
    end
end

-- download() returns a binary Lua string; standard Lua io controls local persistence.
local data = ftp:download("hello.txt")
local output <close> = assert(io.open("ftp-download.txt", "wb"))
output:write(data)

-- Remove all remote data created by this demo.
ftp:rename("hello.txt", "renamed.txt")
ftp:delete("renamed.txt")

ftp:cd("..")
ftp:rmdir(directory)

ftp:quit()
