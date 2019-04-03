--- test timestamping


local mg     = require "moongen"
local lm     = require "libmoon"
local device = require "device"
local memory = require "memory"
local ts     = require "timestamping"
local hist   = require "histogram"
local timer  = require "timer"
local log    = require "log"
local stats  = require "stats"
local arp    = require "proto.arp"
local server = require "webserver"

local RUN_TIME = 5

function configure(parser)
    parser:description("Demonstrate and test hardware timestamping capabilities.")
    parser:argument("dev", "Devices to use."):args(1):convert(tonumber)
    parser:option("-o --output", "File to output statistics to")
    return parser:parse()
end

function master(args)
    args.dev = device.config{port = args.dev, txQueues = 1, dropEnable = false}
    device.waitForLinks()
    local txQueue = args.dev:getTxQueue(0)
    mg.startTask("timestampTest", txQueue)

    print("reach here")

    mg.waitForTasks()
end


function timestampTest(txQueue)
    local timestamper = newTxTimestamper(txQueue)
end





--- Create a new timestamper.
--- A NIC can only be used by one thread at a time due to clock synchronization.
--- Best current pratice is to use only one timestamping thread to avoid problems.
function newTxTimestamper(txQueue, mem, udp, doNotConfigureUdpPort)
    mem = mem or memory.createMemPool(function(buf)
        -- defaults are good enough for us here
        buf:getUdpPtpPacket():fill{ethSrc = txQueue}
        -- consider udp as true
    end)
    txQueue:enableTimestamps()
    return setmetatable({
            mem = mem,
            txBufs = mem:bufArray(1),
            txQueue = txQueue,
            txDev = txQueue.dev,
            seq = 1,
            udp = udp,
            doNotConfigureUdpPort = doNotConfigureUdpPort
        }, timestamper)
end

