math.randomseed(os.time())

function read_params()
    local f = io.open('/tmp/wrk_sendfile.txt')
    set_size = f:read('*number')
    chunk_size = f:read('*number')
    chunks = bit.lshift(1, set_size - chunk_size)
    f:close()
end

read_params()

request = function()
    local idx = math.random(chunks) - 1
    local path = string.format('/data/%d/%d.bin', chunk_size, idx)
    return wrk.format(nil, path)
end
