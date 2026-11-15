-- Configure a TCP client port named "FTP" and use an account with write permission.
-- An SSL client port can be used for an implicit FTPS server instead.
local ftp = Ftp.new("FTP", 30000)

ftp:login("anonymous", "anonymous@example.com")

print("remote working directory", ftp:pwd())

-- Work inside a disposable remote directory so existing files are not modified.
local directory = "unicomm-ftp-demo-" .. os.time()
ftp:mkdir(directory)
ftp:cd(directory)

-- Upload text into the new directory.
ftp:upload("hello.txt", "Hello from UniComm FTP!\n")

-- Optional metadata queries; stat() requires MLST support on the server.
-- print("hello.txt exists", ftp:exists("hello.txt"))
-- local entry = ftp:stat("hello.txt")
-- print(entry.name, entry.type, entry.size, entry.modified)

-- Listing returns structured entries, or raw lines on older servers.
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
