-- Configure a TCP client port named "FTP" and use an account with write permission.
-- SSL client ports are also accepted for implicit TLS; temporary data sockets
-- follow the control port type.
local ftp = Ftp.new("FTP", 30000)

-- SSL login also sends PBSZ 0 and PROT P to enable TLS on data connections.
ftp:login("anonymous", "anonymous@example.com")

-- PWD and other metadata commands use the persistent FTP control connection.
print("remote working directory", ftp:pwd())

-- Work inside a disposable remote directory so existing files are not modified.
local directory = "unicomm-ftp-demo-" .. os.time()
ftp:mkdir(directory)
ftp:cd(directory)

-- Transfer commands open a temporary passive data connection automatically.
-- EPSV is preferred and PASV is used as the fallback.
ftp:upload("hello.txt", "Hello from UniComm FTP!\n")
print("hello.txt exists", ftp:exists("hello.txt"))

-- stat() returns one FileInfo through MLST on the control connection.
local entry = ftp:stat("hello.txt")
print(entry.name, entry.type, entry.size, entry.modified)

-- list() returns multiple FileInfo values through MLSD on a data connection.
local entries = ftp:list()
for _, item in ipairs(entries) do
    print(item.name, item.type, item.size, item.modified)
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
